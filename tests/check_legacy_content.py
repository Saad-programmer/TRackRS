#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Preserve baseline content files and check all legacy XML/ZIP containers.

Read-only, no extraction or network. Owns no application state. This static
check complements the real-engine scenarios; it does not claim every stage
has been driven or that every referenced optional texture exists.
"""
import json
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    inventory = json.loads((ROOT / "tests/baselines/content-inventory.json").read_text())
    counts = {"xml": 0, "zip": 0}
    for group, files in inventory["files"].items():
        for name in files:
            path = ROOT / name
            if not path.is_file():
                raise RuntimeError("baseline content removed: " + name)
            if path.suffix in (".level", ".event", ".vehicle"):
                expected = {".level": "level", ".event": "event", ".vehicle": "vehicle"}[path.suffix]
                if ET.parse(path).getroot().tag != expected:
                    raise RuntimeError("wrong XML root: " + name)
                counts["xml"] += 1
            elif path.suffix == ".zip":
                with zipfile.ZipFile(path) as archive:
                    bad = archive.testzip()
                    if bad:
                        raise RuntimeError("ZIP CRC failed: " + name + ": " + bad)
                counts["zip"] += 1
    print(f"Legacy content: {counts['xml']} XML definitions parsed; "
          f"{counts['zip']} plugin archive CRC checks passed")


if __name__ == "__main__":
    main()
