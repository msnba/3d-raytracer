# 3d-renderer

_A simple C++ 3D renderer using OpenGL._

### Prerequisites 📝

- [A C++ Compiler](https://code.visualstudio.com/docs/languages/cpp#_install-a-compiler)
- [CMake](https://cmake.org)
- [vcpkg](https://vcpkg.io/en)
- [Ninja](https://ninja-build.org/)

### Building 🔨

1. Clone the repo:
   - `git clone https://github.com/msnba/3d-renderer.git`
2. CD into the new directory:
   - `cd ./3d-renderer`
3. Install dependencies (make sure vcpkg is added to PATH):
   - `vcpkg install`
4. Get the cmake toolchain file path for vcpkg:
   - `vcpkg integrate install`
5. Configure project with CMake:
   - `cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Ninja"`
6. Build the project with CMake:
   - `cmake --build build`

## License

**[MIT](https://choosealicense.com/licenses/mit/)**
