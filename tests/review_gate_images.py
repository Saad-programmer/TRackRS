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

def image_metrics(width, height, before, after):
    errors = [abs(a-b) for a,b in zip(before,after)]
    pixel_errors = [max(errors[i:i+3]) for i in range(0,len(errors),3)]
    ordered = sorted(pixel_errors)
    hud = [errors[(y*width+x)*3+c] for y in range(height)
           if y < int(height*.24) or y >= int(height*.65)
           for x in range(width) for c in range(3)]
    scores = []
    for by in range(0,height,8):
        for bx in range(0,width,8):
            a=[];b=[]
            for y in range(by,min(by+8,height)):
                for x in range(bx,min(bx+8,width)):
                    i=(y*width+x)*3
                    a.append(.2126*before[i]+.7152*before[i+1]+.0722*before[i+2])
                    b.append(.2126*after[i]+.7152*after[i+1]+.0722*after[i+2])
            ma=sum(a)/len(a);mb=sum(b)/len(b)
            va=sum((v-ma)**2 for v in a)/len(a);vb=sum((v-mb)**2 for v in b)/len(b)
            covariance=sum((u-ma)*(v-mb) for u,v in zip(a,b))/len(a)
            scores.append(((2*ma*mb+6.5025)*(2*covariance+58.5225))/
                          ((ma*ma+mb*mb+6.5025)*(va+vb+58.5225)))
    return {'rgb_mean_absolute_error':sum(errors)/len(errors),
            'pixel_max_channel_p99':ordered[int(.99*(len(ordered)-1))],
            'pixels_over_24_fraction':sum(e>24 for e in pixel_errors)/len(pixel_errors),
            'block_ssim':sum(scores)/len(scores),
            'hud_mean_absolute_error':sum(hud)/len(hud)}


def within_tolerance(metrics, limits):
    return (metrics['rgb_mean_absolute_error'] <= limits['rgb_mean_absolute_error_max']
            and metrics['pixel_max_channel_p99'] <= limits['pixel_max_channel_p99_max']
            and metrics['pixels_over_24_fraction'] <= limits['pixels_over_24_fraction_max']
            and metrics['block_ssim'] >= limits['block_ssim_min']
            and metrics['hud_mean_absolute_error'] <= limits['hud_mean_absolute_error_max'])


def main():
    p=argparse.ArgumentParser();p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--require-identical',action='store_true');p.add_argument('--tolerance',type=Path);args=p.parse_args();limits=json.loads(args.tolerance.read_text()) if args.tolerance else None
    args.output.mkdir(parents=True,exist_ok=True);records=[];paired=[];rows=[];width=height=None
    for before in sorted(args.before.glob('*.png')):
        after=args.after/before.name
        if not after.exists():raise RuntimeError('missing '+str(after))
        w,h,a=read_png(before);w2,h2,b=read_png(after)
        if (w,h)!=(w2,h2):raise RuntimeError('size drift')
        if width is None:width,height=w,h
        if (width,height)!=(w,h):raise RuntimeError('mixed dimensions')
        equal=a==b;records.append({'stage':before.stem,'before_rgb_sha256':hashlib.sha256(a).hexdigest(),'after_rgb_sha256':hashlib.sha256(b).hexdigest(),'identical':equal})
        if limits:
            records[-1]['metrics']=image_metrics(w,h,a,b)
            records[-1]['within_tolerance']=within_tolerance(records[-1]['metrics'],limits)
        shutil.copy2(before,args.output/(before.stem+'-before.png'));shutil.copy2(after,args.output/(before.stem+'-after.png'))
        paired.append(b''.join(a[y*w*3:(y+1)*w*3]+b[y*w*3:(y+1)*w*3] for y in range(h)))
        name=html.escape(before.stem);rows.append(f'<tr><th>{name}</th><td><img src="{name}-before.png"></td><td><img src="{name}-after.png"></td><td>{equal}</td></tr>')
    if not records:raise RuntimeError('no images')
    write_png(args.output/'side-by-side.png',width*2,height*len(paired),b''.join(paired))
    (args.output/'review.html').write_text('<!doctype html><meta charset="utf-8"><title>Golden review</title><h1>Accepted baseline / gate output</h1><table><tr><th>Stage</th><th>Before</th><th>After</th><th>RGB identical</th></tr>'+''.join(rows)+'</table>')
    (args.output/'review.json').write_text(json.dumps(records,indent=2)+'\n')
    if args.require_identical and not all(r['identical'] for r in records):raise RuntimeError('golden image drift')
    if limits and not all(r['within_tolerance'] for r in records):raise RuntimeError('golden image tolerance exceeded; inspect review.json')
    print('PASS paired golden review:',len(records),'images; exact matches:',sum(r['identical'] for r in records))
if __name__=='__main__':main()
