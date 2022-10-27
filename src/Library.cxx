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
#include "lua/Error.hxx"
#include "lua/Util.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "io/MakeDirectory.hxx"
#include "io/RecursiveDelete.hxx"
#include "system/Error.hxx"

extern "C" {
#include <fox/cp.h>
}

extern "C" {
#include <lauxlib.h>
}

#include <fcntl.h> // for AT_FDCWD

static int
l_make_directory(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "Invalid parameter count");

	if (!lua_isstring(L, 1))
		luaL_argerror(L, 1, "path expected");

	const char *path = lua_tostring(L, 1);

	try {
		NewLuaPathDescriptor(L,
				     MakeNestedDirectory(FileDescriptor{AT_FDCWD}, path),
				     path);
	} catch (...) {
		Lua::RaiseCurrent(L);
	}

	return 1;
}

static int
l_recursive_copy(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "Invalid parameter count");

	if (!lua_isstring(L, 1))
		luaL_argerror(L, 1, "path expected");

	if (!lua_isstring(L, 2))
		luaL_argerror(L, 2, "path expected");

	const char *source_path = lua_tostring(L, 1);
	const char *destination_path = lua_tostring(L, 2);

	constexpr unsigned options = FOX_CP_DEVICE|FOX_CP_INODE;

	try {
		fox_error_t error;
		fox_status_t status = fox_copy(source_path, destination_path, options,
					       &error);
		if (status != FOX_STATUS_SUCCESS)
			throw FormatErrno(status, "Copying '%s' failed", error.pathname);
	} catch (...) {
		Lua::RaiseCurrent(L);
	}

	return 0;
}

static int
l_recursive_delete(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "Invalid parameter count");

	if (!lua_isstring(L, 1))
		luaL_argerror(L, 1, "path expected");

	const char *path = lua_tostring(L, 1);

	try {
		RecursiveDelete(FileDescriptor{AT_FDCWD}, path);
	} catch (...) {
		Lua::RaiseCurrent(L);
	}

	return 0;
}

void
OpenLibrary(lua_State *L) noexcept
{
	Lua::SetGlobal(L, "make_directory", l_make_directory);
	Lua::SetGlobal(L, "recursive_copy", l_recursive_copy);
	Lua::SetGlobal(L, "recursive_delete", l_recursive_delete);
}
