// Copyright (c) 2017-2026 The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "pch.h"
#include "common.h"
#include "graphicsplugin.h"

std::shared_ptr<IGraphicsPlugin> CreateGraphicsPlugin(const std::string graphicsPluginName) {
    if (EqualsIgnoreCase(graphicsPluginName, "OpenGL")) {
        return CreateGraphicsPlugin_OpenGL();
    }
    throw std::invalid_argument(Fmt("Unsupported graphics API '%s'; only OpenGL is available", graphicsPluginName.c_str()));
}
