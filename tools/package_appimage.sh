#!/usr/bin/env bash
# Build an AppImage from the same CMake install rules as ZIP/DEB.
# linuxdeploy and its AppImage output plugin must be supplied by the caller;
# no downloads or package-manager mutations occur here.
set -euo pipefail
build_dir=$(realpath "${1:-build/linux-release}")
if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
  echo 'Expected a configured TrackRS CMake build directory.' >&2
  exit 2
fi
stage=$(mktemp -d "$build_dir/AppDir.XXXXXX")
trap 'rm -rf "$stage"' EXIT
if [[ ! -f "${LDAI_RUNTIME_FILE:-}" ]]; then
  echo 'Set LDAI_RUNTIME_FILE to the checksum-verified AppImage runtime.' >&2
  exit 2
fi
if [[ ! -x "${LINUXDEPLOY:-}" ]]; then
  echo 'Set LINUXDEPLOY to an executable linuxdeploy (with AppImage output support).' >&2
  exit 2
fi
DESTDIR="$stage" cmake --install "$build_dir" --prefix /usr
# Rootless SDK packages are not visible to dpkg-query; retain their license texts.
if [[ -n "${TRACKRS_SYSROOT:-}" && -d "$TRACKRS_SYSROOT/usr/share/doc" ]]; then
  mkdir -p "$stage/usr/share/doc/sdk-dependencies"
  cp -a "$TRACKRS_SYSROOT/usr/share/doc/." "$stage/usr/share/doc/sdk-dependencies/"
fi
cd "$build_dir"
APPIMAGE_EXTRACT_AND_RUN=1 "$LINUXDEPLOY" --appdir "$stage" \
  --desktop-file "$stage/usr/share/applications/trackrs.desktop" \
  --icon-file "$stage/usr/share/pixmaps/trackrs.png" --output appimage
