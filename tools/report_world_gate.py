#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Assemble raw measured evidence and labelled screenshot thumbnails."""
import argparse,json,shutil
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
source=a.input;out=a.output
for file in source.iterdir():
 if file.suffix in ('.json','.png'):shutil.copyfile(file,out/file.name)
before=ROOT/'docs/reports/phase3a/benchmark/ahead-pbr.json'
for suffix in ('.json','.png'):
 shutil.copyfile(before.with_suffix(suffix),out/('before-3a'+suffix))
shutil.copyfile(Path(str(before)+'.terrain.json'),out/'before-3a.terrain.json')
keys=['legacy','dawn-csm','noon-csm','golden-hour-csm','dusk-csm','night-csm','midnight-csm','noon-blob']
records=[('3a PBR fixed lighting',json.loads(before.read_text()))]+[(k,json.loads((source/(k+'.json')).read_text())) for k in keys]
text='''# Before/after world rendering — software measurements\n\nAhead,1920×1080High,120ticks/119samples, identical baseline inputs and vehicle. All states equal3eebeb4e28341a5d. Sequential isolated llvmpipe LLVM19 runs; no concurrent local build/test during the after captures. The accepted3a before record is from its earlier isolated run on this environment. Loading frame excluded; no further warmup exclusion. Cold program/JIT and first-ready sky-bake costs are not representative warm hardware costs; the engine stage-load timer does not include all lazy GPU/JIT work. Software GPU queries are not reference-hardware results.\n\n|Path/time|Draws/frame|Triangles/frame|Frame mean ms|GPU frame ms|p99 ms|Peak RSS MiB|Stage-load ms|\n|---|---:|---:|---:|---:|---:|---:|---:|\n'''
for name,j in records:text+=f"|{name}|{j['draw_calls']:.2f}|{j['triangles']:.0f}|{j['frame_ms_avg']:.2f}|{j['gpu_frame_ms_avg']:.2f}|{j['frame_ms_p99']:.2f}|{j['ram_peak_bytes']/1048576:.1f}|{j['load_ms']:.0f}|\n"
passes=['sky','terrain','shadow_cascade_0','shadow_cascade_1','shadow_cascade_2','shadow_cascade_3','shadow_terrain','sky_ibl_bake','mesh_submit','water','particles','ui_text']
text+='\n## GPU ms per frame\n\n|Path/time|'+'|'.join(passes)+'|\n|---|'+'---:|'*len(passes)+'\n'
for name,j in records:
 values=[j['passes'].get(k,{}).get('gpu_ms_per_frame') for k in passes]
 text+='|'+name+'|'+'|'.join('—' if v is None else f'{v:.4f}' for v in values)+'|\n'
text+='\nScope queries can nest: shadow_terrain is inside cascade scopes, and probes can be inside mesh submission. Do not sum overlapping pass times. Raw records contain CPU scope times and all sample counts. A dash means no measured scope, not a claim of zero execution cost.\n\n## Last-frame geometry and allocated logical shadow payload\n\nThese sidecars count the final rendered frame, while the table above averages119frames. Terrain excludes retained foliage. Cascade counts include terrain, vehicles and alpha-cutout foliage. Nominal depth payload excludes driver padding; D24 may occupy32bits physically. Total VRAM is unavailable portably.\n\n|Time|Main terrain draws/tris|Cascade draws|Cascade triangles|Blob draws|CSM payload bytes|\n|---|---:|---|---|---:|---:|\n'
for key in keys[1:]:
 w=json.loads((source/(key+'.json.world.json')).read_text());t=json.loads((source/(key+'.json.terrain.json')).read_text())
 text+=f"|{key}|{t['draw_calls']}/{t['submitted_triangles']}|{w['cascade_draw_calls']}|{w['cascade_triangles']}|{w['blob_draw_calls']}|{w['shadow_payload_bytes']}|\n"
text+='''\n## Budgets and review\n\nAll after cases meet the master prompt's High geometry limits of≤1500draws and≤3Mtriangles. High90fps/≤11msframe,≤6msCPU/≤10msGPU and60fps1%low, total3GiBVRAM, and warm-hitch/load targets still require the user's real-GPU measurements. Software frame times do not establish those targets. RSS/payload are process/logical metrics, not complete physical GPU allocation. Low720p45fps and Ultra4K75fps are also unverified on reference hardware.\n\n[Six preset thumbnails](times-of-day.png), [3a versus3b Noon](before-after.png), and each full-resolution PNG are actual engine captures. New appearance intentionally changes; unchanged-golden requirements apply to Legacy. Screenshot review and exact legacy comparison are recorded in the gate report.\n'''
(out/'PERFORMANCE.md').write_text(text)
canvas=Image.new('RGB',(1280,1170),(20,20,20));draw=ImageDraw.Draw(canvas)
for i,key in enumerate(keys[1:7]):
 im=Image.open(source/(key+'.png'));im.thumbnail((640,360));x=i%2*640;y=i//2*390;canvas.paste(im,(x,y+30));draw.text((x+10,y+8),key,fill='white')
canvas.save(out/'times-of-day.png')
canvas=Image.new('RGB',(1280,390),(20,20,20));draw=ImageDraw.Draw(canvas)
for i,(path,label) in enumerate([(before.with_suffix('.png'),'Phase 3a PBR fixed lighting'),(source/'noon-csm.png','Phase 3b PBR Noon CSM')]):
 im=Image.open(path);im.thumbnail((640,360));canvas.paste(im,(i*640,30));draw.text((i*640+10,8),label,fill='white')
canvas.save(out/'before-after.png')
print('Saved raw measurements, full-resolution captures, labelled thumbnails and budget tables:',out)
