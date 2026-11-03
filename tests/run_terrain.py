#!/usr/bin/env python3
"""Exercise legacy/PBR worlds with identical CPU replay; never infer hardware FPS."""
import argparse,json,os,subprocess,tempfile
from pathlib import Path
from run_baselines import STAGES,CARS,png_from_ppm
from review_gate_images import read_png
from run_asan import audit,read_policy
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
p.add_argument('--stages',nargs='+',choices=STAGES,default=['ahead','glacier'])
p.add_argument('--frames',type=int,default=32);p.add_argument('--width',type=int,default=640);p.add_argument('--height',type=int,default=480)
p.add_argument('--asan',action='store_true');p.add_argument('--no-probe-case',action='store_true')
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
(a.output/'result.json').unlink(missing_ok=True)
env=dict(os.environ,LIBGL_ALWAYS_SOFTWARE='1',ALSOFT_DRIVERS='null',ASAN_OPTIONS='detect_leaks=1:fast_unwind_on_malloc=0:malloc_context_size=64:halt_on_error=1',LSAN_OPTIONS='exitcode=23:max_leaks=0',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
def run(stage,vehicle,renderer,name,frames):
 target=(a.output/(name+'.json')).resolve();image=target.with_suffix('.ppm')
 for path in (target,image,Path(str(target)+'.terrain.json'),Path(str(target)+'.pbr.json')):path.unlink(missing_ok=True)
 command=[str(a.binary.resolve()),'--headless','--data-dir',str(ROOT/'data'),'--stage',stage,'--vehicle',vehicle,'--renderer',renderer,'--quality','high','--width',str(a.width),'--height',str(a.height),'--frames',str(frames),'--replay',str(ROOT/'tests/replays/baseline.csv'),'--json',str(target),'--screenshot',str(image)]
 log=a.output/(name+'.log')
 with tempfile.TemporaryDirectory(prefix='trackrs-terrain-') as profile:
  with log.open('w') as stream:
   result=subprocess.run(command,env=dict(env,XDG_DATA_HOME=profile),stdout=stream,stderr=subprocess.STDOUT,timeout=900)
 text=log.read_text();accepted=[]
 if a.asan and result.returncode==23:accepted=audit(text,read_policy(ROOT/'lsan.supp'))
 elif result.returncode or 'runtime error:' in text or 'ERROR: AddressSanitizer' in text:raise RuntimeError(f'{name} failed; inspect {log}')
 report=json.loads(target.read_text())
 if report['ticks']!=frames or report['samples']!=frames-1:raise RuntimeError('incorrect terrain replay work count')
 png_from_ppm(image,target.with_suffix('.png'));image.unlink()
 width,height,_=read_png(target.with_suffix('.png'))
 if (width,height)!=(a.width,a.height):raise RuntimeError('benchmark viewport differs from requested dimensions')
 if renderer=='pbr':
  terrain=json.loads(Path(str(target)+'.terrain.json').read_text())
  if terrain['renderer']!='cdlod' or not terrain['ready'] or terrain['nodes_culled']<=0:raise RuntimeError('new terrain was not ready/culling')
  if terrain['cache_tiles']>2048:raise RuntimeError('terrain cache exceeded capacity')
  if report['draw_calls']>1500 or report['triangles']>3000000:raise RuntimeError('submitted geometry exceeds master-prompt High budget')
 return {'name':name,'state_hash':report['state_hash'],'native_exit':result.returncode,'accepted_driver_allocations':accepted,'frame_ms':report['frame_ms_avg'],'draw_calls':report['draw_calls'],'triangles':report['triangles'],'passes':report['passes']}
results=[]
for stage in a.stages:
 vehicle=CARS[STAGES.index(stage)%len(CARS)]
 before=run(stage,vehicle,'legacy',stage+'-legacy',a.frames)
 after=run(stage,vehicle,'pbr',stage+'-pbr',a.frames)
 if before['state_hash']!=after['state_hash']:raise RuntimeError(f'graphics changed CPU physics for {stage}')
 results.extend([before,after]);print(f'PASS: {stage}, legacy/PBR replay hash {after["state_hash"]}',flush=True)
if not a.no_probe_case:
 frames=min(a.frames,60)
 results.append(run('ahead','pbr_test','pbr','car-probe-high',frames))
 profile=json.loads((a.output/'car-probe-high.json.pbr.json').read_text());terrain=json.loads((a.output/'car-probe-high.json.terrain.json').read_text())
 if profile['probe']['size']!=128 or profile['probe']['resident_bytes']!=2162656:raise RuntimeError('High car probe must be128-square')
 if profile['probe']['updates']>frames or terrain['probe_capture_calls']>frames*6:raise RuntimeError('probe refreshed more than once per frame')
 times=profile['probe']['update_times']
 if any(b-a<.25-1e-6 for a,b in zip(times,times[1:])):raise RuntimeError('probe exceeded4Hz')
 if profile['probe']['updates']<1:raise RuntimeError('probe did not refresh')
 for material in profile['materials']:
  if material['gpu_samples']!=frames-1:raise RuntimeError('car material instancing regressed')
(a.output/'result.json').write_text(json.dumps({'passed':True,'viewport':[a.width,a.height],'frames':a.frames,'renderer':'software llvmpipe; not reference hardware','timing_scope':'functional verification; concurrent machine load possible; no hardware performance claim','runs':results},indent=2)+'\n')
print('PASS: terrain GPU integration, unchanged CPU replay and requested High car probe')
