#!/usr/bin/env bash
#
# SimpleDualSheath release packager (macOS LLVM cross-compile).
#
# Builds the Release DLL, verifies it, and zips a mod-manager-ready archive:
#   dist/SimpleDualSheath v<version>.zip
#     Data/SKSE/Plugins/SimpleDualSheath.dll
#     Data/SKSE/Plugins/SimpleDualSheath.ini
#     README.md
#
# Usage:
#   scripts/package-release.sh [--skip-build]
#
# Writes only inside this repo (never into game / mod-manager directories).

set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO"

VERSION="$(sed -n 's/^project(.*VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt | head -1)"
if [[ -z "$VERSION" ]]; then
    echo "[ERROR] Could not parse project version from CMakeLists.txt" >&2
    exit 1
fi

ARCHIVE="dist/SimpleDualSheath v${VERSION}.zip"
DLL="build/SimpleDualSheath.dll"
INI="dist/SimpleDualSheath.ini"

SKIP_BUILD=0
for arg in "$@"; do
    case "$arg" in
        --skip-build) SKIP_BUILD=1 ;;
        *) echo "[ERROR] Unknown argument: $arg (usage: $0 [--skip-build])" >&2; exit 1 ;;
    esac
done

if [[ $SKIP_BUILD -eq 0 ]]; then
    echo "[1/5] Configuring + building Release..."
    cmake -B build -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=/Volumes/S512/Projects/ETC/winsdk/toolchain/clang-cl-msvc.cmake \
        "-DCMAKE_PREFIX_PATH=/Volumes/S512/Projects/ETC/vcpkg/installed/x64-windows-cross" \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_SKYRIM_SE=ON -DENABLE_SKYRIM_AE=ON -DENABLE_SKYRIM_VR=OFF > /dev/null
    cmake --build build
else
    echo "[1/5] Skipping build (--skip-build)"
fi

echo "[2/5] Verifying DLL..."
if [[ ! -f "$DLL" ]]; then
    echo "[ERROR] Missing $DLL (build it first)" >&2
    exit 1
fi
file "$DLL" | grep -q "PE32+.*x86-64" || { echo "[ERROR] Not a 64-bit PE DLL" >&2; exit 1; }
for sym in SKSEPlugin_Load SKSEPlugin_Version SKMP_GetPluginInterface; do
    llvm-readobj --coff-exports "$DLL" 2>/dev/null | grep -q "Name: $sym" \
        || { echo "[ERROR] Missing export: $sym" >&2; exit 1; }
done
echo "      PE32+ x86-64, exports OK"

echo "[3/5] Staging package..."
STAGE="$(mktemp -d "${TMPDIR:-/tmp}/sds-pkg.XXXXXX")"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/Data/SKSE/Plugins"
cp "$DLL" "$STAGE/Data/SKSE/Plugins/"
cp "$INI" "$STAGE/Data/SKSE/Plugins/"
cp README.md "$STAGE/"
if compgen -G "build/*.pdb" > /dev/null; then
    cp build/*.pdb "$STAGE/Data/SKSE/Plugins/"
fi

echo "[4/5] Creating $ARCHIVE..."
mkdir -p dist
rm -f "$ARCHIVE"
(cd "$STAGE" && zip -qr "$REPO/$ARCHIVE" Data README.md)
unzip -l "$ARCHIVE"

echo "[5/5] SHA256:"
shasum -a 256 "$ARCHIVE"
echo "[OK] Release ready: $ARCHIVE"
