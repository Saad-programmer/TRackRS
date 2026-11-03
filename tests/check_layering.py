#!/usr/bin/env python3
"""Reject reverse application dependencies, including transitive local headers.

Core is standalone; platform, world, sim, audio, UI and renderer may share legacy
engine interfaces, but cannot depend on the game/application composition layer.
This is an explicit transitional layering boundary, not a claim of a new RHI.
"""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[1]
HEADERS = ROOT / "src/include"
GAME = {"main.h", "codriver.h", "option.h", "ghost.h"}
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)

def check(path, chain=()):
    if path in chain:
        return
    text = path.read_text()
    for name in INCLUDE.findall(text):
        if Path(name).name in GAME:
            raise RuntimeError("higher-layer include: " + " -> ".join(str(p.relative_to(ROOT)) for p in (*chain, path)) + " -> " + name)
        target = path.parent / name
        if not target.exists():
            target = HEADERS / name
        if not target.exists():
            target = ROOT / name
        if target.exists():
            target = target.resolve()
            relative = target.relative_to(ROOT).as_posix()
            origin = (chain[0] if chain else path).relative_to(ROOT).as_posix()
            forbidden = relative.startswith("engine/application/")
            if origin.startswith("engine/core/"):
                forbidden = not relative.startswith("engine/core/")
            if forbidden:
                raise RuntimeError(f"higher-layer include: {origin} -> {relative}")
            check(target, (*chain, path))

for folder in ("src/PEngine", "src/PSim", "engine/core", "engine/renderer", "engine/rhi", "engine/render", "engine/media", "engine/mesh", "engine/pbr", "engine/assets"):
    for path in (ROOT / folder).rglob("*"):
        if path.suffix in (".cpp", ".h"):
            check(path)
print("PASS: engine modules cannot include application/game headers")
