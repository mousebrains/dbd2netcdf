#! /usr/bin/env python3
#
# This is an updated rewrite of my mkOne bash script
#
# It goes through the input files that fit the following patterns:
#  *.[DdEeMmNnSsTt]?[Dd]   Dinkum binary data files
#  *.[Pp][Dd]0             PD0 ADCP files
#
# builds netCDF files into a target directory
#
# Oct-2023, Pat Welch, pat@mousebrains.com

from argparse import ArgumentParser
import re
import os
import subprocess
import logging
import threading
import time
import queue

class RunCommand(threading.Thread):
    # Class-level queue to collect exceptions from all threads
    __exception_queue = queue.Queue()

    def __init__(self, name:str, cmd:list, ofn:str, filenames:list) -> None:
        threading.Thread.__init__(self)
        self.name = name
        self.__cmd = cmd
        self.__ofn = ofn
        self.__filenames = filenames

    def run(self): # Called on start
        try:
            stime = time.time()
            processCommand(self.__cmd, self.__ofn, self.__filenames)
            logging.info("Took %s wall clock seconds", "{:.2f}".format(time.time() - stime))
        except Exception as e:
            logging.exception("Executing")
            # Push exception to queue so main thread can detect it
            self.__exception_queue.put((self.name, e))

    @classmethod
    def check_exceptions(cls):
        """Check if any thread raised an exception and re-raise it"""
        if not cls.__exception_queue.empty():
            name, exception = cls.__exception_queue.get()
            raise RuntimeError(f"Thread {name} failed") from exception


def printLines(lines:bytes, logger=logging.info) -> None:
    if not lines: return
    for line in lines.split(b"\n"):
        try:
            logger("%s", str(line, "UTF-8"))
        except (UnicodeDecodeError, UnicodeError):
            logger("%s", line)

def processCommand(cmd:list, ofn:str, filenames) -> int:
    logging.info("%s files", len(filenames))

    # Make sure the directory of the output file exists
    odir = os.path.dirname(ofn)
    if not os.path.isdir(odir):
        logging.info("Creating %s", odir)
        os.makedirs(odir, mode=0o755, exist_ok=True)

    cmd.extend(filenames)

    sp = subprocess.run(cmd, shell=False,
                        stderr=subprocess.STDOUT,
                        stdout=subprocess.PIPE,
                        )
    if sp.returncode:
        logging.error("Processing %s", " ".join(cmd))
        printLines(sp.stdout, logging.warning)
    else:
        printLines(sp.stdout, logging.info)
    return sp.returncode

def extractSensors(filenames:list, args:ArgumentParser) -> list:
    cmd = [os.path.join(args.bindir, "dbdSensors"),
           "--cache", args.cache,
           ]
    cmd.extend(filenames)

    sp = subprocess.run(cmd,
                        shell=False,
                        capture_output=True,
                        )
    if sp.returncode:
        logging.error("Processing %s", " ".join(cmd))
        printLines(sp.stderr, logging.warning)
        return None
    if not sp.stdout:
        logging.warning("No sensors found for %s", " ".join(cmd))
        return None

    sensors = []
    for line in sp.stdout.split(b"\n"):
        if not line: continue
        try:
            line = str(line, "UTF-8")
            fields = line.split()
            sensors.append(fields[2])
        except (UnicodeDecodeError, IndexError) as e:
            logging.warning("Unable to parse line %s: %s", line, e)
    return sensors

def processAll(filenames:list, args:ArgumentParser, suffix:str, sensorsFilename:str=None) -> None:
    # Process Dinkum Binary files into a NetCDF,
    # selecting on sensors if specified

    filenames = list(filenames) # ensure it is a list
    if not filenames: return # Nothing to do

    ofn = args.outputPrefix + suffix # Output filename

    cmd = [os.path.join(args.bindir, "dbd2netCDF"),
           "--cache", args.cache,
           "--output", ofn,
           ]
    if args.verbose: cmd.append("--verbose")
    if args.repair: cmd.append("--repair")
    if not args.keepFirst: cmd.append("--skipFirst")
    if args.compression is not None:
        cmd.extend(["--compression", str(args.compression)])
    if sensorsFilename: cmd.extend(["--sensorOutput", sensorsFilename])
    if args.exclude:
        for mission in args.exclude: cmd.extend(["--skipMission", mission])
    if args.include:
        for mission in args.include: cmd.extend(["--keepMission", mission])

    rc = RunCommand(suffix, cmd, ofn, filenames)
    rc.start()

def writeSensors(sensors:set, ofn:str) -> None:
    odir = os.path.dirname(ofn)
    if not os.path.isdir(odir):
        logging.info("Creating %s", odir)
        os.makedirs(odir, mode=0o755, exist_ok=True)

    with open(ofn, "w") as fp:
        fp.write("\n".join(sorted(sensors)))
        fp.write("\n")
    return ofn

def processDBD(filenames:list, args:ArgumentParser) -> None: # Process flight Dinkum Binary files
    filenames = list(filenames)
    if not filenames: return # Nothing to do
    allSensors = set(extractSensors(filenames, args))
    dbdSensors = set(filter(lambda x: x.startswith("m_") or x.startswith("c_"), allSensors))
    sciSensors = set(filter(lambda x: x.startswith("sci_"), allSensors))
    otroSensors = allSensors.difference(dbdSensors).difference(sciSensors)
    sciSensors.add("m_present_time")
    otroSensors.add("m_present_time")
    allFN = writeSensors(allSensors, args.outputPrefix + "dbd.all.sensors")
    dbdFN = writeSensors(dbdSensors, args.outputPrefix + "dbd.sensors")
    sciFN = writeSensors(sciSensors, args.outputPrefix + "dbd.sci.sensors")
    otroFN = writeSensors(otroSensors, args.outputPrefix + "dbd.other.sensors")

    processAll(filenames, args, "dbd.nc", dbdFN)
    processAll(filenames, args, "dbd.sci.nc", sciFN)
    processAll(filenames, args, "dbd.other.nc", otroFN)

def processPD0(filenames:list, args:ArgumentParser, suffix:str="pd0.nc") -> None:
    # Process ADCP PD0 files
    filenames = list(filenames)
    if not filenames: return # Empty list to process

    ofn = args.outputPrefix + suffix # Output filename
    cmd = [os.path.join(args.bindir, "pd02netCDF"),
           "--output", ofn]
    if args.verbose: cmd.append("--verbose")

    rc = RunCommand(suffix, cmd, ofn, filenames)
    rc.start()

parser = ArgumentParser()
parser.add_argument("filename", type=str, nargs="+", help="Dinkum binary files to convert")

grp = parser.add_argument_group(description="dbd2netCDF related arguments")
grp.add_argument("--bindir", type=str, default="/usr/local/bin",
                 help="Path to dbd2netCDF commands")
grp.add_argument("--cache", type=str, default="cache", help="Directory for sensor cache files")
grp.add_argument("--verbose", action="store_true", help="Verbose output")
grp.add_argument("--repair", action="store_true", help="Should corrupted files be 'repaired'")
grp.add_argument("--keepFirst", action="store_true",
                 help="Should the first record not be discarded on all the files?")
grp.add_argument("--compression", type=int, default=None, choices=range(10),
                 metavar="[0-9]",
                 help="Zlib compression level (0=none, 9=max)")
g = grp.add_mutually_exclusive_group()
g.add_argument("--exclude", type=str, action="append", help="Mission(s) to exclude")
g.add_argument("--include", type=str, action="append", help="Mission(s) to include")

grp = parser.add_argument_group(description="Output related arguments")
grp.add_argument("--outputPrefix", type=str, required=True, help="Output prefix")

args = parser.parse_args()

logging.basicConfig(
        format="%(asctime)s %(threadName)s %(levelname)s: %(message)s",
        level=logging.DEBUG if args.verbose else logging.INFO,
        )

if args.exclude is None and args.include is None:
    args.exclude = ( # Default missions to exclude
            "status.mi",
            "lastgasp.mi",
            "initial.mi",
            "overtime.mi",
            "ini0.mi",
            "ini1.mi",
            "ini2.mi",
            "ini3.mi",
            )

args.cache = os.path.abspath(os.path.expanduser(args.cache))
args.bindir = os.path.abspath(os.path.expanduser(args.bindir))

if not os.path.isdir(args.cache):
    logging.info("Creating %s", args.cache)
    os.makedirs(args.cache, mode=0o755, exist_ok=True)

files = list(map(lambda x: os.path.abspath(os.path.expanduser(x)), args.filename))

processAll(filter(lambda x: re.search(r"[.]s[bc]d", x, re.IGNORECASE), files),
           args, "sbd.nc") # Flight decimated Dinkum Binary files
processAll(filter(lambda x: re.search(r"[.]t[bc]d", x, re.IGNORECASE), files),
           args, "tbd.nc") # Science decimated Dinkum Binary files
processAll(filter(lambda x: re.search(r"[.]m[bc]d", x, re.IGNORECASE), files),
           args, "mbd.nc") # Flight decimated Dinkum Binary files
processAll(filter(lambda x: re.search(r"[.]n[bc]d", x, re.IGNORECASE), files),
           args, "nbd.nc") # Science decimated Dinkum Binary files
processDBD(filter(lambda x: re.search(r"[.]d[bc]d", x, re.IGNORECASE), files),
           args) # Flight Dinkum Binary files
processAll(filter(lambda x: re.search(r"[.]e[bc]d", x, re.IGNORECASE), files),
           args, "ebd.nc") # Science Dinkum Binary files

processPD0(filter(lambda x: x.lower().endswith("pd0"), files), args) # Process PD0 files

# Wait for the children to finish
for thrd in threading.enumerate():
    if thrd != threading.main_thread():
        thrd.join()

# Check if any thread failed
RunCommand.check_exceptions()
