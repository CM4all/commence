// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "CommandLine.hxx"
#include "Library.hxx"
#include "Path.hxx"
#include "Random.hxx"
#include "config.h"
#include "io/MakeDirectory.hxx"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"

#ifdef HAVE_SODIUM
#include "PwHash.hxx"
#endif

#ifdef HAVE_JSON
#include "io/FdReader.hxx"
#include "lua/json/Push.hxx"
#include "lua/json/ToJson.hxx"

#include <nlohmann/json.hpp>
#endif

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

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <fcntl.h> // for AT_FDCWD
#include <stdlib.h>

static void SetupLuaState(lua_State *L) {
    luaL_openlibs(L);
#ifdef HAVE_JSON
    Lua::InitToJson(L);
#endif
#ifdef HAVE_MARIADB
    Lua::MariaDB::Init(L);
#endif
    RegisterLuaPath(L);
    RegisterLuaRandom(L);
#ifdef HAVE_SODIUM
    Lua::RegisterPwHash(L);
#endif
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

#ifdef HAVE_JSON

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

#endif // HAVE_JSON

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

#ifdef HAVE_JSON
    if (cmdline.args_json_path != nullptr)
        Lua::SetGlobal(L, "args", LoadJsonFile(cmdline.args_json_path));
#endif
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
