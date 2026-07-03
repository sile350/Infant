#!/usr/bin/env bash
# Portable release for Astra / Debian-like Linux (no Qt install required on client machine).
#
# Prerequisites on BUILD machine only:
#   - Release build in build-astra/ (cmake .. -DCMAKE_BUILD_TYPE=Release && make -j)
#   - qmake from the SAME Qt used for cmake (usually: sudo apt install qt5-qmake qtbase5-dev)
#   - linuxdeploy + qt plugin (downloaded automatically on first run)
#
# Usage:
#   cd ~/DokitLab/Infant
#   bash tools/pack-release.sh
#
# Output:
#   dist/Infant/   — folder to zip/tar and send to customer
#   dist/Infant/Infant.sh — launcher (use this or ./Infant after cd)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-astra}"
DIST_DIR="$ROOT/dist"
APPDIR="$DIST_DIR/Infant.AppDir"
RELEASE_DIR="$DIST_DIR/Infant"
TOOLS_CACHE="$ROOT/tools/.cache"

BINARY="$BUILD_DIR/infant"
ASSETS_SRC="$BUILD_DIR/assets"
if [[ ! -f "$BINARY" ]]; then
    BINARY="$BUILD_DIR/Infant/infant"
    ASSETS_SRC="$BUILD_DIR/Infant/assets"
fi

if [[ ! -f "$BINARY" ]]; then
    echo "ERROR: executable not found. Build first:"
    echo "  mkdir -p $BUILD_DIR && cd $BUILD_DIR"
    echo "  cmake .. -DCMAKE_BUILD_TYPE=Release && make -j\$(nproc)"
    exit 1
fi

if [[ ! -d "$ASSETS_SRC" ]]; then
    echo "ERROR: assets not found at $ASSETS_SRC"
    echo "Rebuild infant — POST_BUILD should copy assets next to the binary."
    exit 1
fi

if ! command -v qmake >/dev/null 2>&1; then
    echo "ERROR: qmake not in PATH. Install Qt dev tools on the build machine:"
    echo "  sudo apt install qt5-qmake qtbase5-dev"
    exit 1
fi

mkdir -p "$TOOLS_CACHE"
LINUXDEPLOY="${LINUXDEPLOY:-$TOOLS_CACHE/linuxdeploy-x86_64.AppImage}"
QT_PLUGIN="${QT_PLUGIN:-$TOOLS_CACHE/linuxdeploy-plugin-qt-x86_64.AppImage}"

download_if_missing() {
    local url="$1"
    local dest="$2"
    if [[ ! -x "$dest" ]]; then
        echo "Downloading $(basename "$dest") ..."
        wget -q -O "$dest" "$url"
        chmod +x "$dest"
    fi
}

download_if_missing \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "$LINUXDEPLOY"
download_if_missing \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "$QT_PLUGIN"

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp "$BINARY" "$APPDIR/usr/bin/infant"
chmod +x "$APPDIR/usr/bin/infant"
cp -r "$ASSETS_SRC" "$APPDIR/usr/bin/assets"

# Optional: ship empty data/ — base.db is created on first run
mkdir -p "$APPDIR/usr/bin/data"

cat > "$APPDIR/infant.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Infant
Exec=infant
Icon=infant
Categories=Office;
EOF

# Placeholder icon (linuxdeploy wants an icon file)
if [[ ! -f "$APPDIR/infant.png" ]]; then
    # 1x1 PNG
    printf '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89\x00\x00\x00\nIDATx\x9cc\x00\x01\x00\x00\x05\x00\x01\r\n-\xdb\x00\x00\x00\x00IEND\xaeB`\x82' > "$APPDIR/infant.png"
fi

echo "Bundling Qt libraries into AppDir ..."
export QMAKE="$(command -v qmake)"
export QML_SOURCES_PATHS=""
"$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --output appimage >/dev/null 2>&1 || \
"$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt

# Flat folder for customer (no AppImage — simpler on Astra)
rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

cp -a "$APPDIR/usr/bin/infant" "$RELEASE_DIR/"
cp -a "$APPDIR/usr/bin/assets" "$RELEASE_DIR/"
cp -a "$APPDIR/usr/bin/data" "$RELEASE_DIR/" 2>/dev/null || mkdir -p "$RELEASE_DIR/data"

if [[ -d "$APPDIR/usr/lib" ]]; then
    cp -a "$APPDIR/usr/lib" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/plugins" ]]; then
    cp -a "$APPDIR/usr/plugins" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/translations" ]]; then
    cp -a "$APPDIR/usr/translations" "$RELEASE_DIR/"
fi

cat > "$RELEASE_DIR/Infant.sh" <<'EOF'
#!/usr/bin/env bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$DIR/plugins/platforms"
exec "$DIR/infant" "$@"
EOF
chmod +x "$RELEASE_DIR/Infant.sh"

# Verify no missing libs (except linux-vdso, libpthread, etc.)
echo ""
echo "Checking bundled binary ..."
if ldd "$RELEASE_DIR/infant" | grep -E 'not found|=> /lib|=> /usr/lib' | grep -v 'linux-vdso\|libpthread\|libdl\|libm\|libc\|libgcc\|libstdc++'; then
    echo "WARNING: some libraries still point outside $RELEASE_DIR/lib"
    echo "Run: LD_LIBRARY_PATH=$RELEASE_DIR/lib ldd $RELEASE_DIR/infant"
else
    echo "OK: Qt libs are bundled."
fi

echo ""
echo "Release ready: $RELEASE_DIR"
echo "Send to customer:"
echo "  tar -czvf Infant.tar.gz -C dist Infant"
echo ""
echo "Customer runs:"
echo "  tar -xzf Infant.tar.gz && cd Infant && ./Infant.sh"
