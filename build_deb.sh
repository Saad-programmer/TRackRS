#!/bin/bash
# Script to build Debian package for TrackRS Demo
set -e

# Work in the repository root directory
cd "$(dirname "$0")"

# Define package name and directory
PKG_NAME="trackrs-demo"
PKG_VERSION="0.6.7"
PKG_DIR="${PKG_NAME}_${PKG_VERSION}_amd64"

echo "Creating Debian package structure..."
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/local/games"
mkdir -p "$PKG_DIR/usr/local/share/games/trackrs"
mkdir -p "$PKG_DIR/usr/share/applications"
mkdir -p "$PKG_DIR/usr/share/pixmaps"
mkdir -p "$PKG_DIR/usr/share/metainfo"
mkdir -p "$PKG_DIR/usr/share/doc/$PKG_NAME/screenshots"

# Write control file
cat << 'EOF' > "$PKG_DIR/DEBIAN/control"
Package: trackrs-demo
Version: 0.6.7
Section: games
Priority: optional
Architecture: amd64
Maintainer: TrackRS Dev Team <dev@trackrs.local>
Depends: libsdl2-2.0-0, libsdl2-image-2.0-0, libglew2.2 | libglew2.1 | libglew2.0, libopenal1, libalut0, libphysfs1, libtinyxml2-10 | libtinyxml2-9 | libtinyxml2-8 | libtinyxml2-6a
Description: TrackRS Game Demo
 A single-player 3D rally racing game featuring vehicle physics, terrain and
 weather effects, and co-driver pace notes.
 .
 This demo package features the Ahead map. See the installed documentation
 for license terms, third-party content credits, and source information.
EOF

# Copy compiled binary and configuration definitions
echo "Copying game executable..."
cp bin/trackrs "$PKG_DIR/usr/local/games/trackrs"
cp bin/trackrs.config.defs "$PKG_DIR/usr/local/games/trackrs.config.defs"

# Copy game assets (data directory)
echo "Copying game assets..."
cp -r data/* "$PKG_DIR/usr/local/share/games/trackrs/"

# Do not redistribute plugin archives marked unlicensed in DATA_AUTHORS.txt.
rm -f "$PKG_DIR/usr/local/share/games/trackrs/plugins/"{event-rscup,event-westernchallenge,map-crossmountain,map-ghats,map-greengrounds,map-helicoil,map-icypeak,map-monza,map-mountainpass,map-pulp,map-roundhouse,map-santa,map-serpentine,map-snowlevel,map-snowyhills,map-tea,map-tobago}.zip

# Copy shortcut file and icon
echo "Setting up shortcuts and icons..."
cp data/metainfo/trackrs.desktop "$PKG_DIR/usr/share/applications/trackrs.desktop"
cp data/icon/logo.png "$PKG_DIR/usr/share/pixmaps/trackrs.png"
cp data/metainfo/org.trackrs.TrackRS.metainfo.xml "$PKG_DIR/usr/share/metainfo/org.trackrs.TrackRS.metainfo.xml"

# Install license and bundled-content attribution notices.
cp debian/copyright "$PKG_DIR/usr/share/doc/$PKG_NAME/copyright"
cp doc/COPYING.txt "$PKG_DIR/usr/share/doc/$PKG_NAME/COPYING.txt"
cp doc/DATA_AUTHORS.txt "$PKG_DIR/usr/share/doc/$PKG_NAME/DATA_AUTHORS.txt"

# Keep four credited in-game screenshots available to package catalogs.
cp data/maps/ahead/ss.png "$PKG_DIR/usr/share/doc/$PKG_NAME/screenshots/ahead.png"
cp data/maps/lazyhills/ss.png "$PKG_DIR/usr/share/doc/$PKG_NAME/screenshots/lazyhills.png"
cp data/maps/authentic/ss.png "$PKG_DIR/usr/share/doc/$PKG_NAME/screenshots/authentic.png"
cp data/events/01-triggercup/far/ss.png "$PKG_DIR/usr/share/doc/$PKG_NAME/screenshots/triggercup-far.png"

# Adjust desktop shortcut Exec path if needed
sed -i 's/Exec=trackrs/Exec=\/usr\/local\/games\/trackrs/' "$PKG_DIR/usr/share/applications/trackrs.desktop"

# Set permissions
echo "Setting executable permissions..."
chmod 755 "$PKG_DIR/usr/local/games/trackrs"

# Build package
DEB_FILE="${PKG_NAME}_${PKG_VERSION}_amd64.deb"
echo "Compiling Debian package..."
if command -v dpkg-deb >/dev/null 2>&1; then
	dpkg-deb --build "$PKG_DIR"
else
	echo "dpkg-deb not found; assembling the standard Debian archive with ar and tar..."
	TMP_BUILD_DIR="$(mktemp -d)"
	DEB_TMP="$PWD/${DEB_FILE}.tmp"
	printf '2.0\n' > "$TMP_BUILD_DIR/debian-binary"
	tar --owner=0 --group=0 --numeric-owner -C "$PKG_DIR/DEBIAN" -czf "$TMP_BUILD_DIR/control.tar.gz" .
	tar --owner=0 --group=0 --numeric-owner --exclude=./DEBIAN -C "$PKG_DIR" -cJf "$TMP_BUILD_DIR/data.tar.xz" .
	pushd "$TMP_BUILD_DIR" >/dev/null
	ar r "$DEB_TMP" debian-binary control.tar.gz data.tar.xz
	popd >/dev/null
	mv "$DEB_TMP" "$DEB_FILE"
	rm -rf "$TMP_BUILD_DIR"
fi

# Clean up build directory
rm -rf "$PKG_DIR"

echo "Debian package ${DEB_FILE} generated successfully!"
