/*
 * Copyright 2022 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Library.hxx"
#include "Path.hxx"
#include "Template.hxx"
#include "io/FileWriter.hxx"
#include "io/MakeDirectory.hxx"
#include "io/Open.hxx"
#include "io/RecursiveCopy.hxx"
#include "io/RecursiveDelete.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "lib/fmt/SystemError.hxx"
#include "lua/Error.hxx"
#include "lua/Util.hxx"
#include "util/ScopeExit.hxx"
#include "util/SpanCast.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <sys/mman.h>
#include <sys/stat.h>

static int l_make_directory(lua_State *L) {
    if (lua_gettop(L) != 1)
        return luaL_error(L, "Invalid parameter count");

    const auto path = GetLuaPath(L, 1);

    try {
        NewLuaPathDescriptor(
            L, MakeNestedDirectory(path.directory_fd, path.relative_path),
            GetLuaPathString(L, 1));
    } catch (...) {
        Lua::RaiseCurrent(L);
    }

    return 1;
}

static int l_recursive_copy(lua_State *L) {
    if (lua_gettop(L) != 2)
        return luaL_error(L, "Invalid parameter count");

    const auto source = GetLuaPath(L, 1);
    const auto destination = GetLuaPath(L, 2);

    try {
        RecursiveCopy(source.directory_fd, source.relative_path,
                      destination.directory_fd, destination.relative_path);
    } catch (...) {
        Lua::RaiseCurrent(L);
    }

    return 0;
}

static int l_recursive_delete(lua_State *L) {
    if (lua_gettop(L) != 1)
        return luaL_error(L, "Invalid parameter count");

    const auto path = GetLuaPath(L, 1);

    try {
        RecursiveDelete(path.directory_fd, path.relative_path);
    } catch (...) {
        Lua::RaiseCurrent(L);
    }

    return 0;
}

static int l_copy_template(lua_State *L) try {
    if (lua_gettop(L) != 2)
        return luaL_error(L, "Invalid parameter count");

    const auto source = GetLuaPath(L, 1);
    const auto destination = GetLuaPath(L, 2);

    const auto source_fd =
        OpenReadOnly(source.directory_fd, source.relative_path);

    struct stat st;
    if (fstat(source_fd.Get(), &st) < 0)
        throw FmtErrno("Failed to stat {}", source.relative_path);

    if (st.st_size > 1024 * 1024)
        throw FmtRuntimeError("File too large: {}", source.relative_path);

    std::size_t source_size = st.st_size;

    char *source_data = (char *)mmap(nullptr, source_size, PROT_READ,
                                     MAP_SHARED, source_fd.Get(), 0);
    if (source_data == (char *)-1)
        throw FmtErrno("Failed to map {}", source.relative_path);

    AtScopeExit(source_data, source_size) { munmap(source_data, source_size); };

    madvise(source_data, source_size, MADV_WILLNEED);

    FileWriter writer{destination.directory_fd, destination.relative_path};
    RunTemplate(L, {source_data, source_size},
                [&writer](auto s) { writer.Write(AsBytes(s)); });

    writer.Commit();

    return 0;
} catch (...) {
    Lua::RaiseCurrent(L);
}

void OpenLibrary(lua_State *L) noexcept {
    Lua::SetGlobal(L, "make_directory", l_make_directory);
    Lua::SetGlobal(L, "recursive_copy", l_recursive_copy);
    Lua::SetGlobal(L, "recursive_delete", l_recursive_delete);
    Lua::SetGlobal(L, "copy_template", l_copy_template);
}
