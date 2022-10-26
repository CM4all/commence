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

#include "PathDescriptor.hxx"
#include "lua/Class.hxx"
#include "io/UniqueFileDescriptor.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <new>

#include <string.h>
#include <fcntl.h> // for AT_FDCWD

static constexpr char lua_unique_file_descriptor_class[] = "UniqueFileDescriptor";
using LuaPathDescriptor = Lua::Class<UniqueFileDescriptor,
				     lua_unique_file_descriptor_class>;

static const UniqueFileDescriptor &
CastLuaPathDescriptor(lua_State *L, int idx)
{
	return LuaPathDescriptor::Cast(L, idx);
}

static int
LuaPathDescriptorToString(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "Invalid parameters");

	[[maybe_unused]]
	const auto &fd = CastLuaPathDescriptor(L, 1);

	// TODO implement
	return 0;
}

void
RegisterLuaPathDescriptor(lua_State *L)
{
	using namespace Lua;

	LuaPathDescriptor::Register(L);
	SetTable(L, RelativeStackIndex{-1}, "__tostring", LuaPathDescriptorToString);
	lua_pop(L, 1);
}

UniqueFileDescriptor *
NewLuaPathDescriptor(lua_State *L, UniqueFileDescriptor src)
{
	return LuaPathDescriptor::New(L, std::move(src));
}

FileDescriptor
GetLuaPathDescriptor(lua_State *L, int idx)
{
	if (lua_isnil(L, idx)) {
		return FileDescriptor{AT_FDCWD};
	} else {
		return CastLuaPathDescriptor(L, idx);
	}
}
