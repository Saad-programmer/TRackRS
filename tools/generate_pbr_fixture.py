#!/usr/bin/env python3
"""Generate original CC0 geometry/textures for the Phase 2c fictional test car.
No downloaded art or Phase 2d IBL/KTX assets are used. Reproducible with Python stdlib.
"""
import json,math,pathlib,struct,zlib,binascii
ROOT=pathlib.Path(__file__).resolve().parents[1];OUT=ROOT/'data/vehicles/pbr_test'
def png(path,pixel,size=16):
    def chunk(tag,data):return struct.pack('>I',len(data))+tag+data+struct.pack('>I',binascii.crc32(tag+data)&0xffffffff)
    raw=b''.join(b'\0'+b''.join(bytes(pixel(x,y)) for x in range(size)) for y in range(size))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',size,size,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b''))
for name,color in [('paint',(190,35,15,255)),('rubber',(28,30,32,255)),('glass',(25,45,65,255)),('white',(255,255,255,255))]:png(OUT/f'{name}.png',lambda x,y,c=color:c)
png(OUT/'normal.png',lambda x,y:(128,128,255,255));png(OUT/'tilted-normal.png',lambda x,y:(128,200,233,255));png(OUT/'paint-orm.png',lambda x,y:(255,70+(x%4)*3,190,255));png(OUT/'rubber-orm.png',lambda x,y:(255,210,0,255));png(OUT/'glass-orm.png',lambda x,y:(255,35,80,255))
def scene(path,parts):
    blob=bytearray();views=[];accessors=[];primitives=[]
    def accessor(values,kind,components):
        offset=len(blob);flat=[c for v in values for c in v];blob.extend(struct.pack('<'+'f'*len(flat),*flat));views.append({'buffer':0,'byteOffset':offset,'byteLength':len(flat)*4});a={'bufferView':len(views)-1,'componentType':5126,'count':len(values),'type':kind}
        if components==3:a.update(min=[min(v[i] for v in values) for i in range(3)],max=[max(v[i] for v in values) for i in range(3)])
        accessors.append(a);return len(accessors)-1
    for center,half,material in parts:
        positions=[];normals=[];uv=[]
        # glTF Y up; the game converts +90 degrees about X to Z up.
        for axis in range(3):
            for sign in (-1,1):
                a=(axis+1)%3;b=(axis+2)%3;corners=[]
                for u,v in ((-1,-1),(1,-1),(1,1),(-1,1)):
                    p=list(center);p[axis]+=sign*half[axis];p[a]+=u*half[a];p[b]+=v*half[b];corners.append(p)
                order=(0,1,2,0,2,3) if sign>0 else (0,2,1,0,3,2)
                for k in order:
                    positions.append(corners[k]);n=[0,0,0];n[axis]=sign;normals.append(n);uv.append(((0,0),(1,0),(1,1),(0,1))[k])
        primitives.append({'attributes':{'POSITION':accessor(positions,'VEC3',3),'NORMAL':accessor(normals,'VEC3',3),'TEXCOORD_0':accessor(uv,'VEC2',2)},'material':material})
    materials=[];images=[];textures=[]
    def texture(name):images.append({'uri':name});textures.append({'source':len(images)-1});return {'index':len(textures)-1}
    for name in ('paint','rubber','glass'):
        m={'name':name,'pbrMetallicRoughness':{'baseColorTexture':texture(name+'.png'),'metallicRoughnessTexture':texture(name+'-orm.png')},'normalTexture':texture('normal.png')}
        if name=='paint':m['extensions']={'KHR_materials_clearcoat':{'clearcoatFactor':1,'clearcoatRoughnessFactor':.12}}
        materials.append(m)
    document={'asset':{'version':'2.0','generator':'TrackRS original CC0 test fixture'},'extensionsUsed':['KHR_materials_clearcoat'],'buffers':[{'uri':path.stem+'.bin','byteLength':len(blob)}],'bufferViews':views,'accessors':accessors,'images':images,'textures':textures,'materials':materials,'meshes':[{'primitives':primitives}],'nodes':[{'mesh':0,'name':'Fictional TrackRS PBR test car'}],'scenes':[{'nodes':[0]}],'scene':0}
    path.write_text(json.dumps(document,indent=2)+'\n');path.with_suffix('.bin').write_bytes(blob)
body=[((0,.25,0),(.75,.25,1.65),0),((0,.72,-.25),(.59,.26,.74),2),((0,1.01,-.25),(.62,.035,.8),0),((0,.35,-1.45),(.74,.08,.3),0)]
wheel=[((0,0,0),(.533333333,1,1),1)]
scene(OUT/'body.gltf',body);scene(OUT/'wheel.gltf',wheel)
full=body+[((x,.1,z),(.17,.30,.30),1) for x in (-.75,.75) for z in (-1.05,1.05)]
scene(OUT/'car.gltf',full)
source=(ROOT/'data/vehicles/cordo_kitcar/cordo_kitcar.vehicle').read_text();source=source.replace('name="Cordo"','name="PBR test car"').replace('class="Kit-Car"','class="Material test"').replace('cordo_kitcar_wheel.obj','wheel.gltf').replace('wheelscale="0.033"','wheelscale="1"').replace('cordo_kitcar.obj','body.gltf').replace('scale="0.01"','scale="1"');(OUT/'pbr_test.vehicle').write_text('\n'.join(line.rstrip() for line in source.splitlines())+'\n')
