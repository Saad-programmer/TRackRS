#!/usr/bin/env python3
"""Original CC0 tileable nine-surface PBR fixtures; no downloaded art."""
import argparse,json,math,subprocess
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--bin',type=Path,required=True);p.add_argument('--output',type=Path,default=ROOT/'data/terrain/pbr');a=p.parse_args()
output=a.output;output.mkdir(parents=True,exist_ok=True)
surfaces=[('tarmac',(45,44,46),.82,.10),('gravel',(146,126,101),.92,.28),('dirt',(119,79,45),.94,.19),('grass',(61,105,35),.91,.23),('sand',(205,172,113),.86,.12),('snow',(235,243,250),.78,.07),('ice',(131,182,203),.12,.025),('mud',(72,48,29),.40,.15),('water',(20,61,74),.055,.025)]
def lattice(x,y,period,seed):
 n=((x%period)*73856093)^((y%period)*19349663)^(seed*83492791)
 n=((n^(n>>13))*1274126177)&0xffffffff
 return (n&65535)/32767.5-1

def noise(x,y,seed):
 value=0.0
 for period,weight in [(4,.40),(8,.30),(16,.20),(32,.10)]:
  u=x*period/64;v=y*period/64;ix=math.floor(u);iy=math.floor(v)
  fx=u-ix;fy=v-iy;fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy)
  a=lattice(ix,iy,period,seed);b=lattice(ix+1,iy,period,seed)
  c=lattice(ix,iy+1,period,seed);d=lattice(ix+1,iy+1,period,seed)
  value+=((a+(b-a)*fx)*(1-fy)+(c+(d-c)*fx)*fy)*weight
 return value

for layer,(name,colour,rough,strength) in enumerate(surfaces):
 images=[Image.new('RGB',(64,64)) for _ in range(3)]
 for y in range(64):
  for x in range(64):
   detail=noise(x,y,layer+31)
   dx=noise(x+1,y,layer+31)-noise(x-1,y,layer+31)
   dy=noise(x,y+1,layer+31)-noise(x,y-1,layer+31)
   nx=-dx*strength*2;ny=-dy*strength*2;length=math.sqrt(nx*nx+ny*ny+1)
   amplitude=[.04,.25,.18,.22,.08,.03,.04,.13,.02][layer]
   images[0].putpixel((x,y),tuple(max(0,min(255,round(c*(1+amplitude*detail)))) for c in colour))
   images[1].putpixel((x,y),(round((nx/length*.5+.5)*255),round((ny/length*.5+.5)*255),round((1/length*.5+.5)*255)))
   images[2].putpixel((x,y),(255,round(min(1,max(.04,rough+.04*detail))*255),0))
 for image,semantic in zip(images,['albedo','normal','orm']):
  source=output/f'{name}-{semantic}.png';image.save(source)
  subprocess.run([str(a.bin.resolve()/'bake-material'),'--input',str(source),'--semantic',semantic,'--output',str(source.with_suffix('.ktx2'))],check=True)
 manifest={'name':name,'albedo':f'{name}-albedo.ktx2','normal':f'{name}-normal.ktx2','orm':f'{name}-orm.ktx2','atlas':False}
 (output/f'{name}.material.json').write_text(json.dumps(manifest,indent=2)+'\n')
(output/'surfaces.json').write_text(json.dumps({'layers':[x[0] for x in surfaces],'size':64,'source':'original seeded periodic fractal grain; CC0'},indent=2)+'\n')
(output/'LICENSE.txt').write_text('Original TrackRS analytic terrain material inputs and generated textures.\nDedicated to the public domain under CC0 1.0 Universal.\nhttps://creativecommons.org/publicdomain/zero/1.0/\n')
