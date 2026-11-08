#!/usr/bin/env python3
"""Exercise shared sky/IBL/fog/CSM with identical physics; no hardware FPS claim."""
import argparse,json,os,subprocess,tempfile
from pathlib import Path
from run_baselines import png_from_ppm
from review_gate_images import read_png
from run_asan import audit,read_policy
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
p.add_argument('--width',type=int,default=640);p.add_argument('--height',type=int,default=480)
p.add_argument('--frames',type=int,default=32);p.add_argument('--quality',choices=['low','medium','high','ultra'],default='medium');p.add_argument('--times',nargs='+',choices=['dawn','noon','golden-hour','dusk','night','midnight'],default=['noon','night']);p.add_argument('--asan',action='store_true')
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);(a.output/'result.json').unlink(missing_ok=True)
env=dict(os.environ,LIBGL_ALWAYS_SOFTWARE='1',ALSOFT_DRIVERS='null',ASAN_OPTIONS='detect_leaks=1:fast_unwind_on_malloc=0:malloc_context_size=64:halt_on_error=1',LSAN_OPTIONS='exitcode=23:max_leaks=0',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
results=[]
def run(name,renderer,time,shadows,vegetation='3d',weather='level',post='on'):
 target=(a.output/(name+'.json')).resolve();image=target.with_suffix('.ppm');target.unlink(missing_ok=True)
 command=[str(a.binary.resolve()),'--headless','--data-dir',str(ROOT/'data'),'--renderer',renderer,'--quality',a.quality,'--width',str(a.width),'--height',str(a.height),'--time-of-day',time,'--shadows',shadows,'--vegetation',vegetation,'--weather',weather,'--post',post,'--stage','ahead','--vehicle','cordo_kitcar','--frames',str(a.frames),'--replay',str(ROOT/'tests/replays/baseline.csv'),'--json',str(target),'--screenshot',str(image)]
 log=a.output/(name+'.log')
 with tempfile.TemporaryDirectory(prefix='trackrs-world-') as profile:
  with log.open('w') as stream:r=subprocess.run(command,env=dict(env,XDG_DATA_HOME=profile),stdout=stream,stderr=subprocess.STDOUT,timeout=900)
 text=log.read_text();accepted=[]
 if a.asan and r.returncode==23:accepted=audit(text,read_policy(ROOT/'lsan.supp'))
 elif r.returncode or 'runtime error:' in text or 'ERROR: AddressSanitizer' in text:raise RuntimeError(f'{name} failed; inspect {log}')
 report=json.loads(target.read_text());assert report['ticks']==a.frames and report['samples']==a.frames-1
 png_from_ppm(image,target.with_suffix('.png'));image.unlink()
 assert read_png(target.with_suffix('.png'))[:2]==(a.width,a.height),'world capture resolution mismatch'
 if renderer=='pbr':
  world=json.loads(Path(str(target)+'.world.json').read_text());assert world['ibl_ready'] and not world['depth_pass']
  assert world['blob_draw_calls']==(0 if shadows=='csm' else 1), 'blob replacement does not match shadow selection'
  assert world['active_cascade_count']==(world['cascade_count'] if shadows=='csm' else 0)
  if shadows=='blob':assert world['shadow_payload_bytes']==0,'blob-only process allocated a CSM texture'
  assert world['csm']==(shadows=='csm');assert world['cascade_count']==(4 if a.quality in ('high','ultra') else 3)
  assert report['draw_calls']<=1500 and report['triangles']<=3000000, 'High submission budgets exceeded'
  for i in range(world['cascade_count']):assert report['passes'][f'shadow_cascade_{i}']['calls']==(a.frames-1 if shadows=='csm' else 0)
  w=world['weather'];assert w['selection']==weather
  pj=Path(str(target)+'.post.json')
  if post=='on':
   pr=json.loads(pj.read_text());assert pr['resolves']>=a.frames-1 and pr['bloom_levels']>0,'post did not resolve every frame'
   assert report['passes']['post']['calls']==a.frames-1
  else:assert not pj.exists() and report['passes']['post']['calls']==0,'post ran while disabled'
  if time in ('night','midnight'):assert world['scene_lights']>=2,'no headlights at night'
  if time=='noon' and weather in ('level','clear'):assert world['scene_lights']==0,'headlights on at clear noon'
  for k in ('wetness','puddles','snow','overcast'):assert 0<=w[k]<=1
  if weather=='rain':assert w['puddles']>0 and w['wetness']>0 and w['overcast']>0
  if weather=='snow':assert w['snow']>0
  if weather=='clear':assert w['wetness']==0 and w['snow']==0
  veg=Path(str(target)+'.vegetation.json')
  if vegetation=='3d':
   v=json.loads(veg.read_text());assert v['draw_calls']>0 and sum(v['instances_per_lod'][:3])>0,'3D vegetation drew no meshes'
   assert report['passes']['vegetation']['calls']==a.frames-1,'vegetation pass not run once per frame'
   assert report['passes']['shadow_vegetation']['calls']==((a.frames-1)*world['cascade_count'] if shadows=='csm' else 0)
   assert v['mesh_bytes']<4*1024*1024 and v['instance_stream_bytes']<8*1024*1024,'vegetation memory budget'
  else:assert not veg.exists() and report['passes']['vegetation']['calls']==0,'cards selection still ran 3D vegetation'
 results.append({'name':name,'state_hash':report['state_hash'],'native_exit':r.returncode,'accepted_driver_allocations':accepted,'draw_calls':report['draw_calls'],'triangles':report['triangles'],'passes':report['passes']})
 print('PASS:',name,report['state_hash'],flush=True)
 return report['state_hash']
reference=run('legacy','legacy','noon','blob')
for time in a.times:assert run(time+'-csm','pbr',time,'csm')==reference,'world lighting changed physics'
assert run('noon-blob','pbr','noon','blob')==reference,'blob selection changed physics'
assert run('noon-cards','pbr','noon','csm','cards')==reference,'vegetation selection changed physics'
assert run('noon-nopost','pbr','noon','csm','3d','level','off')==reference,'post selection changed physics'
for weather in ('rain','snow','clear'):assert run('noon-'+weather,'pbr','noon','csm','3d',weather)==reference,'weather changed physics'
(a.output/'result.json').write_text(json.dumps({'passed':True,'viewport':[a.width,a.height],'frames':a.frames,'quality':a.quality,'timing_scope':'software functional integration; concurrent load possible, not hardware performance','runs':results},indent=2)+'\n')
