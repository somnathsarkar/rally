# Rally

Rally is a game engine written in DirectX 12 and C/C++. It is designed to be fast, implement many features of modern game engines and real-time rendering systems, is built on data-oriented design principles and makes minimal use of third-party libraries and the standard library.

## Features

- Real-time Raytracing powered by DirectX Raytracing (DXR)
- Multithreaded rendering engine using Direct3D 12
- SIMD-accelerated math library with numerical linear algebra routines
- Lockless multithreading with thread pools
- Custom memory allocators with no dynamic memory allocation at runtime
- Win32 platform layer implementing window management, input handling, etc.

## Build Instructions

For supported platforms and build setup information refer to [BUILD.md](BUILD.md)

## License Information

### Third Party Libraries

Rally itself makes use of no third party libraries at runtime, besides the essential Win32 and DirectX headers. However, several libraries are required to support tools for shader compilation, the asset pipeline, etc. License information for them is included below.

#### Official Direct3D 12 headers

[Github](https://github.com/microsoft/DirectX-Headers)

Standard DirectX 12 headers are provided by the Windows SDK (see build instructions). This repository also includes Microsoft's Direct3D 12 helper header `d3dx12.h`, distributed under the [MIT License](rally/external/LICENSE). 

#### Assimp

[Github](https://github.com/assimp/assimp)

Included with repository. Assimp is used by the asset exporter to extract mesh information from common file formats (.dae, .obj, etc.). Modifications to original source include removing documentation, examples and tests. Distributed under the [modified, 3-clause BSD-license](external/assimp/LICENSE).

#### DirectX Shader Compiler

[Github](https://github.com/microsoft/DirectXShaderCompiler)

Must be downloaded separately to modify shaders (see build instructions), distributed under  the terms of the [University of Illinois Open Source License](https://github.com/microsoft/DirectXShaderCompiler/blob/main/LICENSE.TXT).

#### GoogleTest

[Github](https://github.com/google/googletest)

Used to run the included unit tests. Source code is not included in this repostiory, required files are automatically downloaded and built as part of the default build process.