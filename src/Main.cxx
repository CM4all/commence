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

#include "CommandLine.hxx"
#include "Library.hxx"
#include "Path.hxx"
#include "PwHash.hxx"
#include "Random.hxx"
#include "config.h"
#include "io/FdReader.hxx"
#include "io/MakeDirectory.hxx"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"

#include "lua/json/Push.hxx"
#include "lua/json/ToJson.hxx"

#include "lua/Assert.hxx"
#include "lua/RunFile.hxx"
#include "lua/State.hxx"
#include "lua/Util.hxx"
#include "system/Error.hxx"
#include "util/PrintException.hxx"
#include "util/SpanCast.hxx"
#include "util/StringAPI.hxx"

#ifdef HAVE_MARIADB
#include "lua/mariadb/Init.hxx"
#endif

#include <nlohmann/json.hpp>

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <fcntl.h> // for AT_FDCWD
#include <stdlib.h>

static void SetupLuaState(lua_State *L) {
    luaL_openlibs(L);
    Lua::InitToJson(L);
#ifdef HAVE_MARIADB
    Lua::MariaDB::Init(L);
#endif
    RegisterLuaPath(L);
    RegisterLuaRandom(L);
    Lua::RegisterPwHash(L);
    OpenLibrary(L);
}

static std::string GetParentPath(std::string_view path) noexcept {
    auto slash = path.rfind('/');
    if (slash == path.npos)
        return ".";

    if (slash == 0)
        return "/";

    return std::string{path.substr(0, slash)};
}

static nlohmann::json LoadJsonFile(FileDescriptor fd) {
    std::string contents;
    while (true) {
        std::array<std::byte, 16384> buffer;
        auto nbytes = fd.Read(buffer);
        if (nbytes < 0)
            throw MakeErrno("Failed to read JSON file");
        if (nbytes == 0)
            break;
        contents.append(ToStringView(std::span<const std::byte>{buffer}));
    }

    return nlohmann::json::parse(contents);
}

static nlohmann::json LoadJsonFile(const char *path) {
    if (StringIsEqual(path, "-")) {
        return LoadJsonFile(FileDescriptor{STDIN_FILENO});
    } else {
        return LoadJsonFile(OpenReadOnly(path));
    }
}

static void SetGlobals(lua_State *L, const CommandLine &cmdline) {
    const Lua::ScopeCheckStack check_stack{L};

    const auto src = GetParentPath(cmdline.script_path);
    NewLuaPathDescriptor(L, OpenPath(src.c_str(), O_DIRECTORY), src);
    Lua::SetGlobal(L, "src", Lua::RelativeStackIndex{-1});
    lua_pop(L, 1);

    NewLuaPathDescriptor(
        L, MakeDirectory(FileDescriptor{AT_FDCWD}, cmdline.destination_path),
        cmdline.destination_path);
    Lua::SetGlobal(L, "path", Lua::RelativeStackIndex{-1});
    lua_pop(L, 1);

    if (cmdline.args_json_path != nullptr)
        Lua::SetGlobal(L, "args", LoadJsonFile(cmdline.args_json_path));
}

static int Run(const CommandLine &cmdline) {
    const Lua::State lua_state{luaL_newstate()};
    SetupLuaState(lua_state.get());
    SetGlobals(lua_state.get(), cmdline);

    Lua::RunFile(lua_state.get(), cmdline.script_path);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) noexcept try {
    const auto cmdline = ParseCommandLine(argc, argv);
    return Run(cmdline);
} catch (...) {
    PrintException(std::current_exception());
    return EXIT_FAILURE;
}
