#!/usr/bin/env python3
"""Bump the FetchContent GIT_TAGs in CMakeLists.txt to the latest releases.

Run by .github/workflows/dependency-check.yml. Rewrites CMakeLists.txt (and the
dependency list in CLAUDE.md) in place, then leaves the commit message and pull
request body in $RUNNER_TEMP for the workflow to use. The workflow builds and
tests the result before proposing it, so a bump that breaks the tree never
reaches a pull request.

Only strictly-increasing versions are accepted. GitHub's "latest release" is
whatever was published most recently, which is not always the highest version:
a backport patch on an older line, or a project whose release tags do not match
its ref tags, can otherwise walk a pin backwards.
"""

import os
import pathlib
import re
import subprocess
import sys

# GIT_REPOSITORY and GIT_TAG are adjacent lines inside each FetchContent_Declare.
DECLARE = re.compile(
    r"(GIT_REPOSITORY\s+https://github\.com/(?P<repo>\S+?)\.git\s*\n\s*GIT_TAG\s+)(?P<tag>\S+)"
)


def version_key(tag):
    """(1, 17, 0) for 'v1.17.0'; None when the tag is not a plain version."""
    m = re.fullmatch(r"v?(\d+(?:\.\d+)*)", tag)
    return tuple(int(p) for p in m.group(1).split(".")) if m else None


def latest_release(repo):
    proc = subprocess.run(
        ["gh", "api", f"repos/{repo}/releases/latest", "--jq", ".tag_name"],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        print(f"::warning::no release info for {repo}: {proc.stderr.strip()}")
        return None
    return proc.stdout.strip() or None


updates = []


def consider(match):
    repo, current = match.group("repo"), match.group("tag")
    name = repo.rsplit("/", 1)[-1]
    latest = latest_release(repo)
    if latest is None or latest == current:
        print(f"{name}: {current} is up to date")
        return match.group(0)

    old_key, new_key = version_key(current), version_key(latest)
    if old_key is None or new_key is None:
        print(f"::warning::{name}: cannot compare {current} with {latest}; leaving pinned")
        return match.group(0)
    if new_key <= old_key:
        print(f"::warning::{name}: latest release {latest} is not newer than {current}; leaving pinned")
        return match.group(0)

    print(f"{name}: {current} -> {latest}")
    updates.append({"name": name, "repo": repo, "old": current, "new": latest})
    return match.group(1) + latest


cmakelists = pathlib.Path("CMakeLists.txt")
rewritten = DECLARE.sub(consider, cmakelists.read_text())

out = pathlib.Path(os.environ.get("GITHUB_OUTPUT", os.devnull))
if not updates:
    print("All FetchContent dependencies are up to date.")
    with out.open("a") as fh:
        fh.write("has_updates=false\n")
    sys.exit(0)

cmakelists.write_text(rewritten)

# Keep the dependency list in CLAUDE.md in step with the pins.
claude_md = pathlib.Path("CLAUDE.md")
if claude_md.exists():
    text = claude_md.read_text()
    for up in updates:
        text = re.sub(
            rf"^- {re.escape(up['name'])} \S+$",
            f"- {up['name']} {up['new']}",
            text,
            flags=re.MULTILINE,
        )
    claude_md.write_text(text)

if len(updates) == 1:
    up = updates[0]
    subject = f"chore: bump {up['name']} to {up['new']}"
else:
    subject = "chore: bump FetchContent dependencies"

bullets = "\n".join(
    f"- **{u['name']}**: `{u['old']}` -> `{u['new']}` "
    f"([release notes](https://github.com/{u['repo']}/releases/tag/{u['new']}))"
    for u in updates
)

tmp = pathlib.Path(os.environ.get("RUNNER_TEMP", "."))
(tmp / "bump-commit.txt").write_text(f"{subject}\n\n{bullets}\n")
(tmp / "bump-pr.md").write_text(
    f"{bullets}\n\n"
    "Opened by `.github/workflows/dependency-check.yml`, which configured, built "
    "and ran `ctest` against these versions on `ubuntu-latest` before proposing "
    "them. The full compiler matrix runs on merge.\n"
)

with out.open("a") as fh:
    fh.write("has_updates=true\n")
