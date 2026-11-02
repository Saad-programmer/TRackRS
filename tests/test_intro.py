#!/usr/bin/env python3
"""Exercise the in-window intro with real SDL events, then orderly shutdown.
Owns temporary profiles/overlay only; never starts an external media player.
Run on an X display with xdotool; input fixture is generated MPEG, not game art.
"""
import argparse
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--binary', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
args = p.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
for skip in (False, True):
    with tempfile.TemporaryDirectory(prefix='trackrs-intro-') as directory:
        work = Path(directory)
        data = work/'data'
        (data/'policy').mkdir(parents=True)
        (data/'policy/edition.json').write_bytes((ROOT/'data/policy/edition.json').read_bytes())
        (data/'intro.mpg').write_bytes((ROOT/'tests/fixtures/intro.mpg').read_bytes())
        profile = work/'profile'
        (profile/'trackrs').mkdir(parents=True)
        config = ET.parse(ROOT/'bin/trackrs.config.defs')
        paths = config.getroot().find('datadirectory')
        paths.clear()
        ET.SubElement(paths, 'data', path=str(data))
        config.write(profile/'trackrs/trackrs-0.6.7.config')
        path = args.output/('skip.log' if skip else 'play.log')
        env = dict(os.environ, XDG_DATA_HOME=str(profile), ALSOFT_DRIVERS='null', SDL_AUDIODRIVER='dummy', LIBGL_ALWAYS_SOFTWARE='1')
        with path.open('w') as log:
            child = subprocess.Popen([str(args.binary.resolve())], env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic()+30
                while 'In-window pl_mpeg intro started' not in path.read_text() and time.monotonic()<deadline:
                    if child.poll() is not None:
                        raise RuntimeError('intro startup failed')
                    time.sleep(.02)
                if skip:
                    window = subprocess.check_output(['xdotool','search','--onlyvisible','--name','TrackRS'],text=True).splitlines()[0]
                    subprocess.run(['xdotool','key','--window',window,'Escape'],check=True)
                while 'In-window pl_mpeg intro completed or skipped' not in path.read_text() and time.monotonic()<deadline:
                    time.sleep(.02)
                assert 'In-window pl_mpeg intro completed or skipped' in path.read_text(), path
                child.send_signal(signal.SIGTERM)
                child.wait(timeout=15)
                assert child.returncode==0 and 'Shutdown complete' in path.read_text()
                print('PASS intro', 'skip' if skip else 'natural playback', flush=True)
            finally:
                if child.poll() is None:
                    child.kill()
                    child.wait()
