#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise real SDF menu navigation and F12 capture for demo/full/dev policies.
Owns only requested output and temporary profiles. Run on Xvfb with xdotool.
Menu backgrounds animate, so these are visual reviews, not pixel goldens.
"""
from pathlib import Path
import json,os,signal,subprocess,sys,tempfile,time,xml.etree.ElementTree as ET
import argparse
p=argparse.ArgumentParser();p.add_argument('--binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
root=Path(__file__).resolve().parents[1];out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(root/'tests'));from run_baselines import png_from_ppm
def verify_modes():
    for mode in ('demo','full','dev'):
     with tempfile.TemporaryDirectory(prefix='trackrs-edition-') as work:
      work=Path(work);data=work/'data';data.mkdir()
      (data/'policy').mkdir();(data/'policy/edition.json').write_text(json.dumps({'schema':'trackrs.edition/1','mode':mode,'demo_stages':['/maps/ahead/ahead.level']}))
      profile=work/'profile';(profile/'trackrs').mkdir(parents=True)
      config=ET.parse(root/'bin/trackrs.config.defs');directory=config.getroot().find('datadirectory');directory.clear();ET.SubElement(directory,'data',path=str(data))
      video=config.getroot().find('video');video.set('automatic','no');video.set('width','640');video.set('height','480');video.set('fullscreen','no');video.set('vsync','off');video.set('framecap','30')
      config.write(profile/'trackrs/trackrs-0.6.7.config')
      env=dict(os.environ,XDG_DATA_HOME=str(profile),LP_NUM_THREADS='0',ALSOFT_DRIVERS='null',LIBGL_ALWAYS_SOFTWARE='1')
      with (out/(mode+'.log')).open('w') as log:
       process=subprocess.Popen([str(args.binary.resolve())],cwd='/tmp',env=env,stdout=log,stderr=subprocess.STDOUT)
       try:
        time.sleep(6)
        if process.poll() is not None:raise RuntimeError(mode+' startup exited')
        found=subprocess.check_output(['xdotool','search','--onlyvisible','--name','TrackRS'],text=True).splitlines();window=found[0]
        subprocess.run(['xdotool','key','--window',window,'space'],check=True)
        time.sleep(1)
        # Event list is enabled only in full/dev. The demo stays on its top menu.
        subprocess.run(['xdotool','windowfocus','--sync',window],check=True)
        subprocess.run(['xdotool','mousemove','--sync','--window',window,'50','50'],check=True)
        time.sleep(.2)
        subprocess.run(['xdotool','mousemove','--sync','--window',window,'320','160'],check=True)
        time.sleep(.5)
        subprocess.run(['xdotool','click','1'],check=True)
        time.sleep(1)
        subprocess.run(['xdotool','key','--window',window,'F12'],check=True)
        deadline=time.monotonic()+5
        while not list((profile/'trackrs').glob('screen-*.ppm')) and time.monotonic()<deadline:time.sleep(.1)
        time.sleep(.2)
        ppm=next((profile/'trackrs').glob('screen-*.ppm'))
        with ppm.open('rb') as f:
         magic=f.readline();line=f.readline()
         while line.startswith(b'#'):line=f.readline()
         payload=magic+line+f.read()
        normalized=out/(mode+'.ppm');normalized.write_bytes(payload)
        png_from_ppm(normalized,out/(mode+'.png'));normalized.unlink()
        process.send_signal(signal.SIGTERM);process.wait(timeout=10)
        assert process.returncode==0 and 'Shutdown complete' in (out/(mode+'.log')).read_text()
        print('PASS UI startup/click/screenshot/clean exit:',mode,flush=True)
       finally:
        if process.poll() is None:process.kill();process.wait()

try:
    verify_modes()
except Exception as error:
    def annotate(message):
        escaped=message.replace('%','%25').replace('\r','%0D').replace('\n','%0A')
        print('::error::'+escaped,flush=True)
    annotate(type(error).__name__+': '+str(error))
    for path in sorted(out.glob('*.log')):
        lines=path.read_text().splitlines()
        diagnostics=[line for line in lines if any(word in line.lower() for word in ['failed','error','alsa','alut','openal','cannot'])]
        for line in (diagnostics[-12:] or lines[-5:]):annotate(path.name+': '+line)
    raise
