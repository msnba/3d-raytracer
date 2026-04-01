# 3D-Raytracer

_A simple cross-platform C++ 3D Raytracer using OpenGL_

## Screenshots

<p align="center">
  <img src="gh_assets/mirrortest.png" width="49%" />
  <img src="gh_assets/colortest.png" width="49%" />
</p>
<p align="center">
  <img src="gh_assets/modeltest.png" width="50%" />
</p>

## Features ✅

- Real-time Monte Carlo path tracing.
- Ability to load .obj model files.
- Accumulates frames over time for a more realistic picture.
- Helpful user interface.

## Prerequisites 📝

- [A C++ Compiler](https://code.visualstudio.com/docs/languages/cpp#_install-a-compiler)
- [CMake](https://cmake.org)
- [vcpkg](https://vcpkg.io/en)
- [Ninja](https://ninja-build.org/) (Or another build system, just switch up the configure cmd.)
- [OpenGL 4.3+](https://www.opengl.org/)

## Building 🔨

1. Clone the repo:
   - `git clone https://github.com/msnba/3d-renderer.git`
2. CD into the new directory:
   - `cd ./3d-renderer`
3. Allow execution of script:
   - For Linux users: `chmod +x build_and_run.sh`
   - For Windows users (Powershell): `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`
4. Run the script:
   - For Linux users: `./build_and_run.sh`
   - For Windows users (Powershell): `./build_and_run.ps1`

## License

**[MIT](https://choosealicense.com/licenses/mit/)**
