#!/usr/bin/env python3
"""Compile AND link every production PBR permutation with glslang (no GPU needed)."""
import argparse,itertools,json,pathlib,re,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
AXES=('EXPLICIT_BITANGENT','INSTANCED','USE_CSM','ALPHA_TO_COVERAGE')
def expand(path,chain=()):
    if path in chain: raise RuntimeError('cyclic shader include')
    return re.sub(r'^\s*#include "([^"/]+)"\s*$',lambda m:expand(path.parent/m[1],(*chain,path)),path.read_text(),flags=re.M)
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--validator',default='glslangValidator');parser.add_argument('--output');args=parser.parse_args();results=[]
    with tempfile.TemporaryDirectory(prefix='trackrs-pbr-') as temp:
        for flags in itertools.product((False,True),repeat=len(AXES)):
            defines=[axis for axis,flag in zip(AXES,flags) if flag];files=[]
            for stage in ('vert','frag'):
                source=expand(ROOT/f'data/shaders/pbr/pbr.{stage}');source=source.replace('#version 330 core','#version 330 core\n'+''.join(f'#define {d} 1\n' for d in defines),1);path=pathlib.Path(temp)/f'pbr.{stage}';path.write_text(source);files.append(str(path))
            result=subprocess.run([args.validator,'-l',*files],text=True,capture_output=True)
            if result.returncode: raise RuntimeError(f'{defines}: {result.stdout}\n{result.stderr}')
            results.append({'defines':defines,'compile_and_link':'pass'})
    report={'permutations':results}
    if args.output:pathlib.Path(args.output).write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS: {len(results)} PBR shader permutations compile and link')
if __name__=='__main__': main()
