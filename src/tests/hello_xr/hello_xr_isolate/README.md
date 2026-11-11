# hello_xr standalone

This project builds the Windows OpenGL version of hello_xr against an installed
OpenXR SDK. It requires CMake, a C/C++17 compiler, the Windows SDK, and an
OpenXR runtime to run.

Configure and build with the OpenXR SDK installation prefix:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/path/to/openxr-sdk/install"
cmake --build build --config Release
```

Alternatively, set `OpenXR_DIR` to the directory containing
`OpenXRConfig.cmake`. Build the SDK and install its Loader, Headers, and
CMakeConfigs components first if it is not already installed. Use a matching
SDK configuration when linking a static loader. If using a shared loader,
make its DLL available on `PATH` or beside `hello_xr.exe`.

Run with an installed and active OpenXR runtime:

```powershell
.\build\Release\hello_xr.exe -g OpenGL
```