# glad loader provenance

Generated reproducibly with PyPI `glad2==2.0.8` and its bundled registry snapshots:

```
python -m glad --api gl:compatibility=3.3 --extensions GL_KHR_debug,GL_ARB_timer_query,GL_ARB_vertex_buffer_object,GL_EXT_texture_filter_anisotropic --out-path third_party/glad --reproducible c
```

The compatibility declarations exist only for the isolated 2a bridge. The visible
window is verified to have a 3.3-or-later **core** profile at runtime. RHI code
uses core functions. Generated upstream/Khronos license notices remain in the
headers/source. GLEW is no longer used by production code.
