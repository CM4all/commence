// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include "io/FileDescriptor.hxx"

#include <string>
#include <string_view>

struct lua_State;
class FileDescriptor;
class UniqueFileDescriptor;

struct PathReference {
    FileDescriptor directory_fd;
    const char *relative_path;
};

void RegisterLuaPath(lua_State *L) noexcept;

void NewLuaPathDescriptor(lua_State *L, UniqueFileDescriptor src,
                          std::string_view path);

PathReference GetLuaPath(lua_State *L, int idx);

std::string GetLuaPathString(lua_State *L, int idx);
