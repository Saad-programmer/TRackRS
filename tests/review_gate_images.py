#!/usr/bin/env python3
"""Pixel-preserving golden comparison artifact; no rescaling or image synthesis.
Owns the output report directory. PNGs must be the validation runner's RGB/filter-0
captures. Writes paired rows, exact RGB hashes and an HTML side-by-side review.
"""
import argparse
import hashlib
import html
import json
from pathlib import Path
import shutil
import struct
import zlib

def read_png(path):
    content=path.read_bytes()
    if content[:8]!=b'\x89PNG\r\n\x1a\n':raise ValueError('not PNG')
    offset=8;compressed=b'';width=height=0
    while offset<len(content):
        size=struct.unpack('>I',content[offset:offset+4])[0]
        kind=content[offset+4:offset+8];data=content[offset+8:offset+8+size];offset+=12+size
        if kind==b'IHDR':
            width,height,depth,color,_,_,interlace=struct.unpack('>IIBBBBB',data)
            if (depth,color,interlace)!=(8,2,0):raise ValueError('unsupported PNG')
        if kind==b'IDAT':compressed+=data
    data=zlib.decompress(compressed);stride=width*3+1
    if len(data)!=height*stride or any(data[y*stride]!=0 for y in range(height)):raise ValueError('unexpected row filter')
    return width,height,b''.join(data[y*stride+1:(y+1)*stride] for y in range(height))

def write_png(path,width,height,rgb):
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    rows=b''.join(b'\0'+rgb[y*width*3:(y+1)*width*3] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))

def main():
    p=argparse.ArgumentParser();p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--require-identical',action='store_true');args=p.parse_args()
    args.output.mkdir(parents=True,exist_ok=True);records=[];paired=[];rows=[];width=height=None
    for before in sorted(args.before.glob('*.png')):
        after=args.after/before.name
        if not after.exists():raise RuntimeError('missing '+str(after))
        w,h,a=read_png(before);w2,h2,b=read_png(after)
        if (w,h)!=(w2,h2):raise RuntimeError('size drift')
        if width is None:width,height=w,h
        if (width,height)!=(w,h):raise RuntimeError('mixed dimensions')
        equal=a==b;records.append({'stage':before.stem,'before_rgb_sha256':hashlib.sha256(a).hexdigest(),'after_rgb_sha256':hashlib.sha256(b).hexdigest(),'identical':equal})
        shutil.copy2(before,args.output/(before.stem+'-before.png'));shutil.copy2(after,args.output/(before.stem+'-after.png'))
        paired.append(b''.join(a[y*w*3:(y+1)*w*3]+b[y*w*3:(y+1)*w*3] for y in range(h)))
        name=html.escape(before.stem);rows.append(f'<tr><th>{name}</th><td><img src="{name}-before.png"></td><td><img src="{name}-after.png"></td><td>{equal}</td></tr>')
    if not records:raise RuntimeError('no images')
    write_png(args.output/'side-by-side.png',width*2,height*len(paired),b''.join(paired))
    (args.output/'review.html').write_text('<!doctype html><meta charset="utf-8"><title>Golden review</title><h1>Accepted baseline / gate output</h1><table><tr><th>Stage</th><th>Before</th><th>After</th><th>RGB identical</th></tr>'+''.join(rows)+'</table>')
    (args.output/'review.json').write_text(json.dumps(records,indent=2)+'\n')
    if args.require_identical and not all(r['identical'] for r in records):raise RuntimeError('golden image drift')
    print('PASS paired golden review:',len(records),'images; exact matches:',sum(r['identical'] for r in records))
if __name__=='__main__':main()
