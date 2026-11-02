# glad loader provenance

Generated reproducibly with PyPI `glad2==2.0.8` and its bundled registry snapshots:

```
python -m glad --api gl:core=3.3 --extensions GL_KHR_debug,GL_ARB_timer_query,GL_EXT_texture_filter_anisotropic --out-path third_party/glad --reproducible c
```

Phase 2b removes compatibility declarations as well as the shared compatibility
context. The only game context is checked for a 3.3-or-later **core** profile at
runtime. Generated Khronos/upstream notices remain in the files. There are no
removed fixed-function entry points available for production code to call.
