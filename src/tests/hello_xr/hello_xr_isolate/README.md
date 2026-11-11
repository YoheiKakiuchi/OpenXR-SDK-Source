# hello_xr standalone

This project builds a headless OpenXR tracking loop against an installed
OpenXR SDK. It requires CMake and a C++17 compiler. Running it requires an
active OpenXR runtime that supports the `XR_MND_headless` extension; runtimes
without that extension cannot create this graphics-free session.

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

Run with a compatible active OpenXR runtime:

```powershell
.\build\Release\hello_xr.exe
```

Press Ctrl+C to exit. Each frame prints the head and controller grip poses in
the local reference space, followed by one button per hand. Positions are in
meters and orientations are quaternions in x, y, z, w order. `?` indicates an
invalid position or orientation; `inactive` indicates an unavailable button.
Actions are available only when the runtime gives the session input focus.

Suggested button bindings are select for KHR Simple, X/A for Oculus Touch,
menu for Vive and Microsoft Motion, and A for Valve Index. The runtime may
choose different bindings or leave an action inactive.