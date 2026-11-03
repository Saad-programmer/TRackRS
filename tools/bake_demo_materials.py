#!/usr/bin/env python3
"""Rebuild original CC0 Phase2d material art with the native KTX2 tools.
Run from any directory: python tools/bake_demo_materials.py --bin build/linux-release/bin
The accepted 2c PNG fixtures remain available for regression tests.
"""
import argparse,hashlib,json,pathlib,runpy,subprocess,math,struct
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--bin',type=pathlib.Path,required=True);p.add_argument('--skip-ibl',action='store_true');args=p.parse_args();binary=args.bin.resolve()
source=runpy.run_path(str(ROOT/'tools/generate_pbr_fixture.py'));out=source['OUT'];png=source['png'];scene=source['scene']
png(out/'chrome.png',lambda x,y:(238,238,238,255));png(out/'chrome-orm.png',lambda x,y:(255,18,255,255))
png(out/'paint-orm-pbr.png',lambda x,y:(255,90+(x%4)*2,0,255))
png(out/'paint-normal.png',lambda x,y:(128+int(4*math.sin(x*.8)),128+int(4*math.cos(y*.8)),255,255))
png(out/'asphalt.png',lambda x,y:(35+(x*13+y*7)%9,35+(x*13+y*7)%9,35+(x*13+y*7)%9,255),64)
png(out/'asphalt-normal.png',lambda x,y:(128+int(5*math.sin(x*.6)),128+int(5*math.cos(y*.7)),255,255),64)
png(out/'asphalt-orm.png',lambda x,y:(220,100+(x*3+y*11)%12,0,255),64)
scene(out/'wheel.gltf',source['wheel']+[((x,0,0),(.035,.7,.7),3) for x in (-.56,.56)])
scene(out/'car.gltf',source['full']+[((x,.1,z),(.025,.20,.20),3) for x in (-.94,.94) for z in (-1.05,1.05)])
scene(out/'asphalt.gltf',[((0,-.23,0),(12,.02,12),0)])
materials={
 'paint':('paint.png','paint-normal.png','paint-orm-pbr.png'),
 'rubber':('rubber.png','normal.png','rubber-orm.png'),
 'glass':('glass.png','normal.png','glass-orm.png'),
 'chrome':('chrome.png','normal.png','chrome-orm.png'),
 'asphalt':('asphalt.png','asphalt-normal.png','asphalt-orm.png'),
 'white':('white.png','normal.png','white.png')}
for name,images in materials.items():
 for semantic,image in zip(('albedo','normal','orm'),images):
  subprocess.run([str(binary/'bake-material'),'--input',str(out/image),'--semantic',semantic,'--output',str(out/f'{name}-{semantic}.ktx2')],check=True)
 manifest={'name':name,'albedo':f'{name}-albedo.ktx2','normal':f'{name}-normal.ktx2','orm':f'{name}-orm.ktx2','atlas':False}
 (out/f'{name}.material.json').write_text(json.dumps(manifest,indent=2)+'\n')
for path in (out/'body.gltf',out/'wheel.gltf',out/'car.gltf',out/'asphalt.gltf'):
 d=json.loads(path.read_text());d['asset']['generator']='TrackRS Phase 2d original CC0 test art'
 def texture(uri):
  fallbacks={f'{name}-{role}.ktx2':image for name,inputs in materials.items() for role,image in zip(('albedo','normal','orm'),inputs)}
  d['images'].append({'uri':fallbacks[uri],'extras':{'trackrsKtx2':uri}});d['textures'].append({'source':len(d['images'])-1});return {'index':len(d['textures'])-1}
 d['materials']=[]
 for name in ('asphalt',) if path.stem=='asphalt' else ('paint','rubber','glass','chrome'):
  m={'name':{'paint':'clear-coat paint','asphalt':'wet asphalt'}.get(name,name),'pbrMetallicRoughness':{'baseColorTexture':texture(f'{name}-albedo.ktx2'),'metallicRoughnessTexture':texture(f'{name}-orm.ktx2')},'normalTexture':texture(f'{name}-normal.ktx2')}
  m['occlusionTexture']={'index':m['pbrMetallicRoughness']['metallicRoughnessTexture']['index'],'strength':1}
  if name=='paint':m['extensions']={'KHR_materials_clearcoat':{'clearcoatFactor':1,'clearcoatRoughnessFactor':.05}}
  d['materials'].append(m)
 # Remove unused PNG declarations to make the shipping format explicit.
 referenced={v['index'] for m in d['materials'] for v in [m['pbrMetallicRoughness']['baseColorTexture'],m['pbrMetallicRoughness']['metallicRoughnessTexture'],m['normalTexture']]}
 remap={old:new for new,old in enumerate(sorted(referenced))};images=[d['images'][d['textures'][old]['source']] for old in sorted(referenced)];d['images']=images;d['textures']=[{'source':i} for i in range(len(images))]
 for m in d['materials']:
  for v in (m['pbrMetallicRoughness']['baseColorTexture'],m['pbrMetallicRoughness']['metallicRoughnessTexture'],m['normalTexture'],m['occlusionTexture']):v['index']=remap[v['index']]
 if path.stem=='asphalt':
  raw=bytearray(path.with_suffix('.bin').read_bytes())
  for primitive in d['meshes'][0]['primitives']:
   accessor=d['accessors'][primitive['attributes']['TEXCOORD_0']];view=d['bufferViews'][accessor['bufferView']]
   for i in range(accessor['count']*2):
    offset=view.get('byteOffset',0)+i*4;value=struct.unpack_from('<f',raw,offset)[0];struct.pack_into('<f',raw,offset,value*12)
  path.with_suffix('.bin').write_bytes(raw)
 path.write_text(json.dumps(d,indent=2)+'\n')
preview=ROOT/'data/scenes/material_preview';description=json.loads((preview/'scene.json').read_text());description.update(name='Phase 2d PBR materials and local IBL',asphalt='../../vehicles/pbr_test/asphalt.gltf',ibl='ibl',lighting='sky split-sum IBL and point headlights; local HDR reflection probe at 2Hz');(preview/'scene.json').write_text(json.dumps(description,indent=2)+'\n')
if not args.skip_ibl:subprocess.run([str(binary/'ibl-baker'),'--output',str(preview/'ibl'),'--size','64','--samples','256'],check=True)
files=sorted(out.glob('*.ktx2'))+sorted((preview/'ibl').glob('*.ktx2'));manifest={'generator':'tools/bake_demo_materials.py','texture_formats':{'albedo':'BC7 sRGB','normal':'BC5 linear','orm':'BC7 linear; AO/Roughness/Metallic in R/G/B','environment':'BC6H unsigned HDR','lut':'RG16F'},'sha256':{str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest() for f in files}};(preview/'bake-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
