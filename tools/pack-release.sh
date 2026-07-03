#!/usr/bin/env bash
# Portable release for Astra / Debian-like Linux (no Qt install required on client machine).
#
# Customer unpacks archive and runs:  ./infant
# (no shell wrapper — RPATH + qt.conf are baked in)
#
# Prerequisites on BUILD machine only:
#   sudo apt install qt5-qmake qtbase5-dev patchelf wget
#   Release build in build-astra/
#
# Usage:
#   cd ~/DokitLab/Infant
#   bash tools/pack-release.sh

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
    exit 1
fi

if ! command -v qmake >/dev/null 2>&1; then
    echo "ERROR: qmake not in PATH. Install: sudo apt install qt5-qmake qtbase5-dev"
    exit 1
fi

if ! command -v patchelf >/dev/null 2>&1; then
    echo "ERROR: patchelf not found. Install: sudo apt install patchelf"
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
mkdir -p "$APPDIR/usr/bin/data"

cat > "$APPDIR/infant.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Infant
Exec=infant
Icon=infant
Categories=Office;
EOF

if [[ ! -f "$APPDIR/infant.png" ]]; then
    printf '\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1f\x15\xc4\x89\x00\x00\x00\nIDATx\x9cc\x00\x01\x00\x00\x05\x00\x01\r\n-\xdb\x00\x00\x00\x00IEND\xaeB`\x82' > "$APPDIR/infant.png"
fi

echo "Bundling Qt libraries ..."
export QMAKE="$(command -v qmake)"
export QML_SOURCES_PATHS=""
"$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt --output appimage >/dev/null 2>&1 || \
"$LINUXDEPLOY" --appdir "$APPDIR" --plugin qt

rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

cp -a "$APPDIR/usr/bin/infant" "$RELEASE_DIR/"
cp -a "$APPDIR/usr/bin/assets" "$RELEASE_DIR/"
mkdir -p "$RELEASE_DIR/data"

if [[ -d "$APPDIR/usr/lib" ]]; then
    cp -a "$APPDIR/usr/lib" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/plugins" ]]; then
    cp -a "$APPDIR/usr/plugins" "$RELEASE_DIR/"
fi
if [[ -d "$APPDIR/usr/translations" ]]; then
    cp -a "$APPDIR/usr/translations" "$RELEASE_DIR/"
fi

# Qt finds platform plugin (libqxcb.so) via qt.conf next to the executable
cat > "$RELEASE_DIR/qt.conf" <<'EOF'
[Paths]
Prefix = .
Plugins = plugins
Translations = translations
EOF

echo "Patching RPATH (no LD_LIBRARY_PATH / .sh wrapper needed) ..."
patchelf --set-rpath '$ORIGIN/lib' "$RELEASE_DIR/infant"

if [[ -d "$RELEASE_DIR/lib" ]]; then
    while IFS= read -r -d '' sofile; do
        patchelf --set-rpath '$ORIGIN' "$sofile" 2>/dev/null || true
    done < <(find "$RELEASE_DIR/lib" -maxdepth 1 -name '*.so*' -print0)
fi

chmod +x "$RELEASE_DIR/infant"

echo ""
echo "Checking (must work without Infant.sh) ..."
cd "$RELEASE_DIR"
unset LD_LIBRARY_PATH QT_PLUGIN_PATH QT_QPA_PLATFORM_PLUGIN_PATH

missing="$(ldd ./infant | grep 'not found' || true)"
if [[ -n "$missing" ]]; then
    echo "ERROR: missing libraries:"
    echo "$missing"
    ldd ./infant
    exit 1
fi

qt_libs="$(ldd ./infant | grep -i qt5 | head -3 || true)"
if echo "$qt_libs" | grep -q '/usr/lib'; then
    echo "WARNING: Qt still loads from system paths (RPATH may be wrong):"
    echo "$qt_libs"
else
    echo "OK: Qt loads from bundled lib/ ($(echo "$qt_libs" | wc -l) libs checked)."
fi

echo ""
echo "Release ready: $RELEASE_DIR"
echo ""
echo "Send to customer:"
echo "  tar -czvf Infant.tar.gz -C dist Infant"
echo ""
echo "Customer runs (from unpacked folder):"
echo "  chmod +x infant   # once, if needed"
echo "  ./infant"
