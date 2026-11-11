cmake_minimum_required(VERSION 3.16)
project(hello_xr_isolate LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This standalone OpenGL configuration currently supports Windows only.")
endif()

find_package(OpenXR CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(hello_xr
    graphicsplugin_factory.cpp
    graphicsplugin_opengl.cpp
    logger.cpp
    main.cpp
    openxr_program.cpp
    platformplugin_factory.cpp
    platformplugin_win32.cpp
    swapchain_image_data.cpp
    common/gfxwrapper_opengl.c
    external/glad2/src/gl.c
    external/glad2/src/wgl.c
)

target_compile_features(hello_xr PRIVATE cxx_std_17)
target_compile_definitions(hello_xr PRIVATE
    XR_USE_PLATFORM_WIN32
    XR_USE_GRAPHICS_API_OPENGL
    XRDEPENDENCIES_USE_GLAD
    _CRT_SECURE_NO_WARNINGS
)
target_include_directories(hello_xr PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/glad2/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/span-lite/include"
)
target_link_libraries(hello_xr PRIVATE OpenXR::openxr_loader OpenGL::GL ole32)