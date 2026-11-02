#!/usr/bin/env python3
"""Run core RHI integration with native sanitizers and the audited driver policy.
Owns only output logs. No native any-frame suppressions; unknown origins fail.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
from run_asan import audit, read_policy
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--binary', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--asan', action='store_true')
args = p.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
(args.output/'result.json').unlink(missing_ok=True)
env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE='1', ASAN_OPTIONS='detect_leaks=1:fast_unwind_on_malloc=0:malloc_context_size=64:halt_on_error=1', LSAN_OPTIONS='exitcode=23:max_leaks=0', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
with (args.output/'raw.log').open('w') as log:
    result = subprocess.run([str(args.binary.resolve()),str(ROOT/'tests/fixtures/rhi'),str(ROOT/'data/shaders/basic')],env=env,stdout=log,stderr=subprocess.STDOUT,timeout=180)
text = (args.output/'raw.log').read_text()
if 'RHI_GATE_OK:' not in text:
    raise RuntimeError('core integration did not pass; inspect raw.log')
accepted = []
if args.asan and result.returncode==23:
    accepted = audit(text,read_policy(ROOT/'lsan.supp'))
elif result.returncode!=0 or 'runtime error:' in text or 'ERROR: AddressSanitizer' in text:
    raise RuntimeError('core sanitizer/process failure; inspect raw.log')
report = {'schema':'trackrs.core-gate/1','passed':True,'native_exit':result.returncode,
          'accepted_driver_bytes':sum(item['bytes'] for item in accepted),'accepted_driver_allocations':accepted,
          'expected_gl_api_canary':True,'unexpected_application_errors':0}
(args.output/'result.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
