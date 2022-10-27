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
#include "lua/RunFile.hxx"
#include "lua/State.hxx"
#include "lua/Util.hxx"
#include "util/PrintException.hxx"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

#include <stdlib.h>

static void
SetupLuaState(lua_State *L)
{
	luaL_openlibs(L);
	RegisterLuaPath(L);
	OpenLibrary(L);
}

static int
Run(const CommandLine &cmdline)
{
	const Lua::State lua_state{luaL_newstate()};
	SetupLuaState(lua_state.get());

	Lua::SetGlobal(lua_state.get(), "path", cmdline.destination_path);

	Lua::RunFile(lua_state.get(), cmdline.script_path);
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv) noexcept
try {
	const auto cmdline = ParseCommandLine(argc, argv);
	return Run(cmdline);
} catch (...) {
	PrintException(std::current_exception());
	return EXIT_FAILURE;
}
