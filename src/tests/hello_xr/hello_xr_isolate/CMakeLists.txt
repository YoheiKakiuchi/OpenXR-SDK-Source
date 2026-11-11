cmake_minimum_required(VERSION 3.16)
project(hello_xr_isolate LANGUAGES CXX)

find_package(OpenXR CONFIG REQUIRED)
add_executable(hello_xr headless.cpp)
target_compile_features(hello_xr PRIVATE cxx_std_17)
target_link_libraries(hello_xr PRIVATE OpenXR::openxr_loader)