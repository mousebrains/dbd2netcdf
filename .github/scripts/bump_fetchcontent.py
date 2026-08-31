#!/usr/bin/env python3
"""Bump the FetchContent GIT_TAGs in CMakeLists.txt to the latest releases.

Run by .github/workflows/dependency-check.yml. Rewrites CMakeLists.txt, the
dependency list in CLAUDE.md and the Unreleased section of the ChangeLog in
place, then leaves the commit message and pull request body in $RUNNER_TEMP for
the workflow to use. The workflow builds and tests the result before proposing
it, so a bump that breaks the tree never reaches a pull request.

Only strictly-increasing versions are accepted. GitHub's "latest release" is
whatever was published most recently, which is not always the highest version:
a backport patch on an older line, or a project whose release tags do not match
its ref tags, can otherwise walk a pin backwards.
"""

import datetime
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


# A dated topic heading inside the ChangeLog, e.g. "Aug-2026, Dependencies".
TOPIC = re.compile(r"^[A-Z][a-z]{2}-\d{4}, .+$")
RELEASE = re.compile(r"^Version \d")

# Recreated verbatim when a release cut has renamed the previous Unreleased
# heading away and nothing has re-added one. Kept in sync with ChangeLog by
# hand; it is instructions for a human, not something this script consumes.
UNRELEASED_PREAMBLE = [
    "Unreleased",
    "",
    "  Add entries here as changes land. At release time, rename this heading to",
    '  "Version X.Y.Z", bump VERSION to match, and tag. The release workflow',
    '  requires a "Version X.Y.Z" section matching the tag and publishes it as the',
    "  release notes, so an unrenamed or missing section fails the release rather",
    "  than shipping silently.",
    "",
]


def update_changelog(updates, path=pathlib.Path("ChangeLog")):
    """Record the bumps under Unreleased, and say what was done.

    One bullet per dependency, not one per bump: a dependency already listed in
    Unreleased is rewritten in place rather than given a second line, so a pin
    bumped twice between releases reads as the net change the release actually
    ships. That leaves the bullet under the month it first moved, which is the
    honest date for "this changed since the last release".
    """
    if not path.exists():
        return "ChangeLog: not found, skipped"

    lines = path.read_text().split("\n")
    month = datetime.datetime.now(datetime.timezone.utc).strftime("%b-%Y")

    if "Unreleased" in lines:
        start = lines.index("Unreleased")
    else:
        # A release cut renames the heading away; put a fresh one above the
        # newest release section rather than silently dropping the entries.
        start = next((i for i, l in enumerate(lines) if RELEASE.match(l)), len(lines))
        lines[start:start] = UNRELEASED_PREAMBLE
    end = next(
        (i for i in range(start + 1, len(lines)) if RELEASE.match(lines[i])), len(lines)
    )

    pending, revised = [], []
    for up in updates:
        bullet = re.compile(rf"^  - Update {re.escape(up['name'])} to \S+$")
        hit = next((i for i in range(start, end) if bullet.match(lines[i])), None)
        if hit is None:
            pending.append(up)
        else:
            lines[hit] = f"  - Update {up['name']} to {up['new']}"
            revised.append(up["name"])

    if pending:
        bullets = [f"  - Update {u['name']} to {u['new']}" for u in pending]
        heading = f"{month}, Dependencies"
        if heading in lines[start:end]:
            at = lines.index(heading, start, end)
            # The block runs to the next topic heading or the end of Unreleased.
            stop = next(
                (i for i in range(at + 1, end) if TOPIC.match(lines[i])), end
            )
            while stop > at + 1 and not lines[stop - 1].strip():
                stop -= 1
            lines[stop:stop] = bullets
        else:
            tail = end
            while tail > start and not lines[tail - 1].strip():
                tail -= 1
            lines[tail:tail] = ["", heading] + bullets

    path.write_text("\n".join(lines))
    added = ", ".join(u["name"] for u in pending)
    return "ChangeLog: " + "; ".join(
        filter(None, [f"added {added}" if added else "", f"revised {', '.join(revised)}" if revised else ""])
    )


def main():
    cmakelists = pathlib.Path("CMakeLists.txt")
    rewritten = DECLARE.sub(consider, cmakelists.read_text())

    out = pathlib.Path(os.environ.get("GITHUB_OUTPUT", os.devnull))
    if not updates:
        print("All FetchContent dependencies are up to date.")
        with out.open("a") as fh:
            fh.write("has_updates=false\n")
        return

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

    changelog_note = update_changelog(updates)
    print(changelog_note)

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



if __name__ == "__main__":
    main()
