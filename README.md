# Tiny Tracer

_A simple, cross-platform path tracer written in C++ and OpenGL._

## Showcase

<p align="center">
  <img src="gh_assets/mirrortest.png" width="49%" />
  <img src="gh_assets/colortest.png" width="49%" />
</p>
<p align="center">
  <img src="gh_assets/modeltest.png" width="50%" />
</p>

## Features
- Real-time Monte Carlo path tracing.
- A helpful user interface.
- Ability to load model files, scene files, and setting configurations.
- Support for different materials and object properties, such as glass.

## Building Prerequisites

- [A C++ Compiler](https://code.visualstudio.com/docs/languages/cpp#_install-a-compiler)
- [CMake](https://cmake.org)
- [vcpkg](https://vcpkg.io/en)
- [Ninja](https://ninja-build.org/) (optional but recommended)
- A graphics card supporting OpenGL 4.3+.

#### Linux (Fedora / Red Hat)
```bash
sudo dnf install cmake ninja-build gcc gcc-c++ pkgconf-pkg-config \
                 libX11-devel libXrandr-devel libXinerama-devel \
                 libXcursor-devel libXi-devel mesa-libGLU-devel \
                 autoconf automake autoconf-archive
```

#### Linux (Ubuntu / Debian)
```bash
sudo apt update
sudo apt install cmake ninja-build build-essential pkg-config \
                 libx11-dev libxrandr-dev libxinerama-dev \
                 libxcursor-dev libxi-dev libglu1-mesa-dev \
                 autoconf automake autoconf-archive
```

#### Windows (Visual Studio / MSVC)
- [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/) - include *Desktop Development with C++*.
  - Located at the bottom of the page in the *Tools for Visual Studio* section.
- Make sure to include vcpkg and CMake in the components.


## Building 🔨

#### 1. Set up vcpkg (Linux only)
```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
```
Add this to your ~/.bashrc or ~/.zshrc:
```bash
export VCPKG_ROOT=/path/to/your/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```

#### 2. Clone and build
On Windows, use the Developer Command Prompt.
```bash
git clone https://github.com/msnba/3d-raytracer.git
cd 3d-raytracer

# Windows (Developer Command Prompt)
vcpkg integrate install
cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE="/path/to/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static -DCMAKE_BUILD_TYPE=Release

# Linux
cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release
```

## License

**[MIT](https://choosealicense.com/licenses/mit/)**
