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
 A restricted demo version of TrackRS, the 3D rally racing game.
 This demo version only features the longest map ("Ahead").
EOF

# Copy compiled binary and configuration definitions
echo "Copying game executable..."
cp bin/trackrs "$PKG_DIR/usr/local/games/trackrs"
cp bin/trackrs.config.defs "$PKG_DIR/usr/local/games/trackrs.config.defs"

# Copy game assets (data directory)
echo "Copying game assets..."
cp -r data/* "$PKG_DIR/usr/local/share/games/trackrs/"

# Copy shortcut file and icon
echo "Setting up shortcuts and icons..."
cp data/metainfo/trackrs.desktop "$PKG_DIR/usr/share/applications/trackrs.desktop"
cp data/icon/trigger-256.png "$PKG_DIR/usr/share/pixmaps/trackrs.png"

# Adjust desktop shortcut Exec path if needed
sed -i 's/Exec=trackrs/Exec=\/usr\/local\/games\/trackrs/' "$PKG_DIR/usr/share/applications/trackrs.desktop"

# Set permissions
echo "Setting executable permissions..."
chmod 755 "$PKG_DIR/usr/local/games/trackrs"

# Build package
echo "Compiling Debian package..."
dpkg-deb --build "$PKG_DIR"

# Clean up build directory
rm -rf "$PKG_DIR"

echo "Debian package ${PKG_DIR}.deb generated successfully!"
