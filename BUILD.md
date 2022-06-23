# Build Instructions

## System requirements

Rally is built on DirectX 12 and makes use of the DirectX Raytracing (DXR) API. The build environment requires the following:

1. OS: Windows 10 version 1809 or later, 64-bit
2. GPU: AMD Radeon RX 6000 series or later/NVIDIA 10- series or later
3. Compiler: Microsoft Visual Studio 2019 or later
4. Windows SDK: Tested on version 10.0.19041.0
5. CMake: version 3.20+

## Optional dependencies

### DirectX Shader Compiler

Rally includes both source code and compiled headers of all shaders. If you would like to make edits to shader source code and automatically recompile them as part of the build system, you must download the DirectX Shader Compiler (`dxc.exe`) here: [link](https://github.com/microsoft/DirectXShaderCompiler/releases/tag/v1.6.2112). Extract the `dxc_2021_12_08` folder to `rally/external`. The final location of the compiler executable should be `rally/external/dxc_2021_12_08/bin/x64/dxc.exe`

## Build

Rally uses CMake as a build system. By default, the engine and all tools, tests and examples are built together. For convenience, the included `build.bat` runs the build process in the debug configuration and all tests, stopping if an error is encountered. This requires cmake to be available via the `PATH` environment variable.