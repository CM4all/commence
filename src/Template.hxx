// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include <functional>
#include <string_view>

struct lua_State;

using TemplateWriteCallback = std::function<void(std::string_view)>;

void RunTemplate(lua_State *L, std::string_view t,
                 TemplateWriteCallback callback);
