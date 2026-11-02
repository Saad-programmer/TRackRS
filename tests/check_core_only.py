#!/usr/bin/env python3
"""Reject removed GL entry points in active production sources and the loader.
Core APIs such as query begins and vertex-attribute configuration are allowed.
Historical reports/build directories are not active source.
"""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
legacy=re.compile(r'\b(?:gl(?:Begin|End|Vertex[234](?:[a-z]+)|Matrix\w*|Material\w*|VertexPointer|NormalPointer|ColorPointer|TexCoordPointer|ShadeModel|ColorMaterial|EdgeFlag\w*|InterleavedArrays|NewList|EndList|CallList\w*|GenLists|DeleteLists|Light\w*|Fog\w*|TexEnv\w*|AlphaFunc|Color[34]\w*|Normal[3]\w*|TexCoord[1-4]\w*|TexGen\w*|(?:Push|Pop)(?:Matrix|Attrib)|LoadIdentity|MultMatrix\w*|Translate\w*|Rotate\w*|Scale\w*|Frustum|Ortho|(?:Enable|Disable)ClientState)|GL_LIGHTING)\b|SDL_GL_CONTEXT_PROFILE_COMPATIBILITY')
failures=[]
for root in ['src','engine','third_party/glad']:
 for path in (ROOT/root).rglob('*'):
  if path.suffix not in {'.cpp','.c','.h'}:continue
  for number,line in enumerate(path.read_text().splitlines(),1):
   if legacy.search(line):failures.append(f'{path.relative_to(ROOT)}:{number}: {line.strip()}')
if failures:raise SystemExit('\n'.join(failures))
print('PASS: no removed GL entry points or compatibility-context creation in active source/loader')
