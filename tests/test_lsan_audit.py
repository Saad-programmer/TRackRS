#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Adversarial report fixtures: application/callback origins must remain fatal."""
import unittest
from run_asan import audit
SITE = {("libGLX_mesa.so.0", 0x123, "bf78ba17851731a2b44a35894165b350073fbf4a")}


def report(origin, caller="in TrackRS::run /workspace/TRackRS/src/app.cpp:12", size=12, summary=12):
    return f"""ERROR: LeakSanitizer: detected memory leaks
Direct leak of {size} byte(s) in 1 object(s) allocated from:
    #0 0x1 in malloc /src/asan_malloc_linux.cpp:69
    #1 0x2 {origin} (BuildId: bf78ba17851731a2b44a35894165b350073fbf4a)
    #2 0x3 {caller}
SUMMARY: AddressSanitizer: {summary} byte(s) leaked in 1 allocation(s).
"""


class AuditTests(unittest.TestCase):
    def test_driver_origin_with_application_caller_is_explicitly_allowed(self):
        self.assertEqual(audit(report("(/lib/x86_64-linux-gnu/libGLX_mesa.so.0+0x123)"),
                               SITE)[0]["bytes"], 12)

    def test_application_origin_with_driver_caller_is_fatal(self):
        with self.assertRaises(ValueError):
            audit(report("in TrackRS::allocate /workspace/TRackRS/src/app.cpp:5",
                         "(/lib/x86_64-linux-gnu/libGLX_mesa.so.0+0x123)"), SITE)

    def test_unresolved_origin_is_fatal(self):
        with self.assertRaises(ValueError):
            audit(report("(<unknown module>)"), SITE)

    def test_same_named_project_library_is_fatal(self):
        with self.assertRaises(ValueError):
            audit(report("(/workspace/TRackRS/libGLX_mesa.so.0+0x123)"), SITE)

    def test_partial_reports_are_fatal(self):
        with self.assertRaises(ValueError):
            audit(report("(/lib/x86_64-linux-gnu/libGLX_mesa.so.0+0x123)", summary=24),
                  SITE)

    def test_other_sanitizer_error_is_fatal(self):
        with self.assertRaises(ValueError):
            audit(report("(/lib/x86_64-linux-gnu/libGLX_mesa.so.0+0x123)") +
                  "\nERROR: AddressSanitizer: heap-use-after-free\n", SITE)

    def test_unknown_driver_callsite_is_fatal(self):
        with self.assertRaises(ValueError):
            audit(report("(/lib/x86_64-linux-gnu/libGLX_mesa.so.0+0x456)"), SITE)

    def test_unknown_driver_build_is_fatal(self):
        with self.assertRaises(ValueError):
            audit(report("(/lib/x86_64-linux-gnu/libGLX_mesa.so.0+0x123)").replace(
                "bf78ba17851731a2b44a35894165b350073fbf4a", "a" * 40), SITE)


if __name__ == "__main__":
    unittest.main()
