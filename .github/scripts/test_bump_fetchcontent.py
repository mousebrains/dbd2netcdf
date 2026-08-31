#!/usr/bin/env python3
"""Tests for the ChangeLog rewriting in bump_fetchcontent.py.

Run directly: python3 .github/scripts/test_bump_fetchcontent.py

The rest of the script fails loudly -- a bad GIT_TAG breaks the build the
workflow runs before proposing anything. The ChangeLog edit is the one part
that can go wrong quietly, mangling prose in a way no compiler will catch, so
it is the part that carries tests.
"""

import datetime
import importlib.util
import pathlib
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "bump", pathlib.Path(__file__).with_name("bump_fetchcontent.py")
)
bump = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bump)

MONTH = datetime.datetime.now(datetime.timezone.utc).strftime("%b-%Y")

PREAMBLE = """Unreleased

  Add entries here as changes land. At release time, rename this heading to
  "Version X.Y.Z", bump VERSION to match, and tag. The release workflow
  requires a "Version X.Y.Z" section matching the tag and publishes it as the
  release notes, so an unrenamed or missing section fails the release rather
  than shipping silently.
"""

RELEASED = """Version 1.7.6

Aug-2026, Dependencies
  - Update CLI11 to v2.7.2
"""


def catch2(new="v3.16.0"):
    return [{"name": "Catch2", "repo": "catchorg/Catch2", "old": "v3.15.3", "new": new}]


class ChangeLogTests(unittest.TestCase):
    def rewrite(self, text, updates):
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d) / "ChangeLog"
            p.write_text(text)
            note = bump.update_changelog(updates, p)
            return p.read_text(), note

    def test_creates_dated_heading_when_absent(self):
        got, _ = self.rewrite(PREAMBLE + "\n" + RELEASED, catch2())
        self.assertIn(f"{MONTH}, Dependencies\n  - Update Catch2 to v3.16.0\n", got)
        # The released section must be left alone.
        self.assertTrue(got.endswith(RELEASED))

    def test_appends_to_existing_heading_for_this_month(self):
        text = PREAMBLE + f"\n{MONTH}, Dependencies\n  - Update spdlog to v1.17.0\n\n" + RELEASED
        got, _ = self.rewrite(text, catch2())
        self.assertIn(
            f"{MONTH}, Dependencies\n"
            "  - Update spdlog to v1.17.0\n"
            "  - Update Catch2 to v3.16.0\n",
            got,
        )
        # The released section carries an identically-named heading when the
        # bump lands in the same month as the last release. The new bullet must
        # join the Unreleased one and not spawn a second heading beside it.
        unreleased = got[: got.index("Version 1.7.6")]
        self.assertEqual(unreleased.count(f"{MONTH}, Dependencies"), 1)
        self.assertNotIn("Catch2", got[got.index("Version 1.7.6") :])

    def test_second_bump_revises_the_bullet_in_place(self):
        text = PREAMBLE + "\nJul-2026, Dependencies\n  - Update Catch2 to v3.16.0\n\n" + RELEASED
        got, note = self.rewrite(text, catch2(new="v3.16.1"))
        self.assertIn("Jul-2026, Dependencies\n  - Update Catch2 to v3.16.1\n", got)
        self.assertNotIn("v3.16.0", got)
        self.assertIn("revised Catch2", note)

    def test_does_not_touch_a_released_sections_bullet(self):
        # CLI11 v2.7.2 is already shipped in 1.7.6; a new bump must add a fresh
        # Unreleased bullet, not rewrite release history.
        updates = [{"name": "CLI11", "repo": "CLIUtils/CLI11", "old": "v2.7.2", "new": "v2.8.0"}]
        got, _ = self.rewrite(PREAMBLE + "\n" + RELEASED, updates)
        self.assertIn("Version 1.7.6\n\nAug-2026, Dependencies\n  - Update CLI11 to v2.7.2\n", got)
        self.assertIn(f"{MONTH}, Dependencies\n  - Update CLI11 to v2.8.0\n", got)

    def test_recreates_unreleased_after_a_release_cut(self):
        got, _ = self.rewrite(RELEASED, catch2())
        self.assertTrue(got.startswith("Unreleased\n"))
        self.assertIn("Add entries here as changes land.", got)
        self.assertIn(f"{MONTH}, Dependencies\n  - Update Catch2 to v3.16.0\n", got)
        self.assertIn("Version 1.7.6", got)

    def test_preserves_a_multi_line_entry_above_it(self):
        wrapped = (
            PREAMBLE
            + "\nAug-2026, Packaging\n"
            "  - Name binary packages with the architecture as well as the OS\n"
            "    (dbd2netcdf-X.Y.Z-Linux-x86_64, -Darwin-arm64). Windows and the\n"
            "    source packages are unchanged\n\n"
            + RELEASED
        )
        got, _ = self.rewrite(wrapped, catch2())
        self.assertIn("    source packages are unchanged\n", got)
        self.assertLess(got.index("Aug-2026, Packaging"), got.index(f"{MONTH}, Dependencies"))

    def test_multiple_dependencies(self):
        updates = catch2() + [
            {"name": "spdlog", "repo": "gabime/spdlog", "old": "v1.17.0", "new": "v1.18.0"}
        ]
        got, note = self.rewrite(PREAMBLE + "\n" + RELEASED, updates)
        self.assertIn(
            f"{MONTH}, Dependencies\n"
            "  - Update Catch2 to v3.16.0\n"
            "  - Update spdlog to v1.18.0\n",
            got,
        )
        self.assertIn("added Catch2, spdlog", note)

    def test_missing_changelog_is_not_fatal(self):
        with tempfile.TemporaryDirectory() as d:
            note = bump.update_changelog(catch2(), pathlib.Path(d) / "nope")
        self.assertIn("skipped", note)

    def test_trailing_newline_is_preserved(self):
        text = PREAMBLE + "\n" + RELEASED
        got, _ = self.rewrite(text, catch2())
        self.assertTrue(got.endswith("\n"))
        self.assertFalse(got.endswith("\n\n"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
