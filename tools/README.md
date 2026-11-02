# AppImage packaging tool provenance

`package_appimage.sh` uses the same install layout and license exclusions as CPack.
Supply linuxdeploy with its AppImage plugin using `LINUXDEPLOY`.
CI pins linuxdeploy release `1-alpha-20251107-1` from
https://github.com/linuxdeploy/linuxdeploy/releases/tag/1-alpha-20251107-1
with SHA-256 `c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d`
for `linuxdeploy-x86_64.AppImage`. The tool is not committed or linked into the game.
Its bundled license texts remain in the tool's upstream distribution.

The square icon is the existing licensed `data/icon/trigger-256.png`; the wide
TrackRS wordmark cannot serve as a freedesktop square icon without modification.

The AppImage type-2 runtime is supplied with `LDAI_RUNTIME_FILE`. CI verifies
SHA-256 `617aa77314d57c8b8e4fae9c3f0b35af7b30d5529cb69e98f64f92510ed98a46`
for `runtime-x86_64` from
https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64 .
Its upstream rolling URL may change; checksum drift deliberately fails instead of
silently adopting a new executable. Refresh the pinned digest only after review.
No runtime or build-tool executable is committed to this repository.
