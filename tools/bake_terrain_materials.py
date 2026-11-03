#!/usr/bin/env python3
"""Original CC0 tileable nine-surface PBR fixtures; no downloaded art."""
import argparse,json,math,subprocess
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--bin',type=Path,required=True);a=p.parse_args()
output=ROOT/'data/terrain/pbr';output.mkdir(parents=True,exist_ok=True)
surfaces=[('tarmac',(45,44,46),.82,.10),('gravel',(146,126,101),.92,.28),('dirt',(119,79,45),.94,.19),('grass',(61,105,35),.91,.23),('sand',(205,172,113),.86,.12),('snow',(235,243,250),.78,.07),('ice',(131,182,203),.12,.025),('mud',(72,48,29),.40,.15),('water',(20,61,74),.055,.025)]
for layer,(name,colour,rough,strength) in enumerate(surfaces):
 images=[Image.new('RGB',(64,64)) for _ in range(3)]
 for y in range(64):
  for x in range(64):
   u=2*math.pi*x/64;v=2*math.pi*y/64
   detail=.45*math.sin(7*u+3*v)+.30*math.cos(11*v-5*u)+.25*math.sin(17*u+13*v)
   dx=.45*7*math.cos(7*u+3*v)+.30*5*math.sin(11*v-5*u)+.25*17*math.cos(17*u+13*v)
   dy=.45*3*math.cos(7*u+3*v)-.30*11*math.sin(11*v-5*u)+.25*13*math.cos(17*u+13*v)
   nx=-dx*strength*.12;ny=-dy*strength*.12;length=math.sqrt(nx*nx+ny*ny+1)
   images[0].putpixel((x,y),tuple(max(0,min(255,round(c*(1+.18*detail)))) for c in colour))
   images[1].putpixel((x,y),(round((nx/length*.5+.5)*255),round((ny/length*.5+.5)*255),round((1/length*.5+.5)*255)))
   images[2].putpixel((x,y),(255,round(min(1,max(.04,rough+.04*detail))*255),0))
 for image,semantic in zip(images,['albedo','normal','orm']):
  source=output/f'{name}-{semantic}.png';image.save(source)
  subprocess.run([str(a.bin.resolve()/'bake-material'),'--input',str(source),'--semantic',semantic,'--output',str(source.with_suffix('.ktx2'))],check=True)
 manifest={'name':name,'albedo':f'{name}-albedo.ktx2','normal':f'{name}-normal.ktx2','orm':f'{name}-orm.ktx2','atlas':False}
 (output/f'{name}.material.json').write_text(json.dumps(manifest,indent=2)+'\n')
(output/'surfaces.json').write_text(json.dumps({'layers':[x[0] for x in surfaces],'size':64,'source':'original analytic periodic material inputs; CC0'},indent=2)+'\n')
(output/'LICENSE.txt').write_text('Original TrackRS analytic terrain material inputs and generated textures.\nDedicated to the public domain under CC0 1.0 Universal.\nhttps://creativecommons.org/publicdomain/zero/1.0/\n')
