# Copyright (c) 2017-2026 The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

if(NOT XR_USE_GRAPHICS_API_OPENGL)
    message(STATUS "Skipping hello_xr: OpenGL graphics support is unavailable")
    return()
endif()

set(LOCAL_HEADERS
    check.h
    common.h
    geometry.h
    graphicsplugin.h
    logger.h
    openxr_program.h
    options.h
    pch.h
    platformdata.h
    platformplugin.h
    swapchain_image_data.h
)
set(LOCAL_SOURCE
    graphicsplugin_factory.cpp
    graphicsplugin_opengl.cpp
    logger.cpp
    main.cpp
    openxr_program.cpp
    platformplugin_factory.cpp
    swapchain_image_data.cpp
)

if(WIN32)
    list(APPEND LOCAL_SOURCE platformplugin_win32.cpp)
else()
    list(APPEND LOCAL_SOURCE platformplugin_posix.cpp)
endif()

add_executable(hello_xr ${LOCAL_SOURCE} ${LOCAL_HEADERS})
add_sanitizers(hello_xr)

set_target_properties(hello_xr PROPERTIES FOLDER ${SAMPLES_FOLDER})
source_group("Headers" FILES ${LOCAL_HEADERS})

target_link_libraries(hello_xr PRIVATE OpenXR::openxr_loader)

target_include_directories(
    hello_xr
    PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${PROJECT_SOURCE_DIR}/src/common"
        # Backport of std::span functionality to pre-C++17
        ${PROJECT_SOURCE_DIR}/src/external/span-lite/include
)

if(TARGET openxr-gfxwrapper)
    target_link_libraries(hello_xr PRIVATE openxr-gfxwrapper)
endif()
if(WIN32)
    target_link_libraries(hello_xr PRIVATE ole32)
    if(MSVC)
        target_compile_definitions(hello_xr PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(hello_xr PRIVATE /Zc:wchar_t /Zc:forScope /W4)
        if(NOT
           CMAKE_CXX_COMPILER_ID
           STREQUAL
           "Clang"
        )
            # If actually msvc and not clang-cl
            target_compile_options(openxr_c_compile_test PRIVATE /WX)
        endif()

    endif()
endif()

install(
    TARGETS hello_xr RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                             COMPONENT hello_xr
)
if(NOT WIN32)
    install(
        FILES hello_xr.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/
        COMPONENT ManPages
    )
endif()
