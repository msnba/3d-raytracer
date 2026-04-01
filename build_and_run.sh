#!/usr/bin/env bash
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }

check_tool() {
    local cmd="$1" install_hint="$2"
    if ! command -v "$cmd" &>/dev/null; then
        error "'$cmd' not found. $install_hint"
    fi
    success "$cmd found: $(command -v "$cmd")"
}

info "Checking prerequisites..."
check_tool cmake  "Install from https://cmake.org"
check_tool ninja  "Install from https://ninja-build.org  (or 'brew install ninja' / 'sudo apt install ninja-build')"
check_tool vcpkg  "Install from https://vcpkg.io/en and make sure it is on your PATH"

info "Locating vcpkg toolchain file..."

if [[ -n "${VCPKG_ROOT:-}" ]]; then
    TOOLCHAIN="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
elif VCPKG_BIN="$(command -v vcpkg 2>/dev/null)"; then
    VCPKG_ROOT_RESOLVED="$(cd "$(dirname "$(realpath "$VCPKG_BIN" 2>/dev/null || readlink -f "$VCPKG_BIN" 2>/dev/null || echo "$VCPKG_BIN")")" && pwd)"
    TOOLCHAIN="$VCPKG_ROOT_RESOLVED/scripts/buildsystems/vcpkg.cmake"
fi

if [[ -z "${TOOLCHAIN:-}" || ! -f "$TOOLCHAIN" ]]; then
    INTEGRATE_OUT="$(vcpkg integrate install 2>&1 || true)"
    TOOLCHAIN="$(echo "$INTEGRATE_OUT" | grep -o '[^ ]*vcpkg.cmake' | head -1 || true)"
fi

if [[ -z "${TOOLCHAIN:-}" || ! -f "$TOOLCHAIN" ]]; then
    warn "Could not auto-detect the vcpkg toolchain file."
    read -rp "Please enter the full path to vcpkg.cmake: " TOOLCHAIN
fi

[[ -f "$TOOLCHAIN" ]] || error "Toolchain file not found at: $TOOLCHAIN"
success "vcpkg toolchain: $TOOLCHAIN"

info "Installing vcpkg dependencies..."

if [[ -f "vcpkg.json" ]]; then
    info "Found vcpkg.json — installing all manifest dependencies..."
    vcpkg install
else
    VCPKG_PACKAGES=(
        glfw3
        glm
        imgui
        tinyobjloader
        opengl
    )
    warn "No vcpkg.json found. Installing common packages: ${VCPKG_PACKAGES[*]}"
    vcpkg install "${VCPKG_PACKAGES[@]}"
fi

success "vcpkg dependencies installed."

info "Configuring project with CMake..."

cmake \
    -B build \
    -G "Ninja" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DCMAKE_BUILD_TYPE=Release

success "CMake configuration complete."

info "Building project..."
cmake --build build --config Release

success "Build complete."

info "Searching for executable..."

EXECUTABLE=""
for candidate in \
    "./build/3d-renderer" \
    "./build/renderer" \
    "./build/raytracer" \
    "./build/Release/3d-renderer" \
    "./build/Release/renderer" \
    "./build/Release/raytracer" \
    "./build/3d-renderer.exe" \
    "./build/Release/3d-renderer.exe"
do
    if [[ -f "$candidate" ]]; then
        EXECUTABLE="$candidate"
        break
    fi
done

if [[ -z "$EXECUTABLE" ]]; then
    EXECUTABLE="$(find ./build -maxdepth 2 -type f -perm /111 ! -name '*.cmake' ! -name '*.sh' | head -1 || true)"
fi

if [[ -z "$EXECUTABLE" ]]; then
    warn "Could not automatically locate the built executable."
    warn "Look inside ./build/ and run it manually."
    exit 0
fi

success "Found executable: $EXECUTABLE"
info "Launching 3D Raytracer..."
exec "$EXECUTABLE"