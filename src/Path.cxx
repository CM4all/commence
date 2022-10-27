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

#include "Path.hxx"
#include "lua/Class.hxx"
#include "lua/Value.hxx"
#include "io/UniqueFileDescriptor.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <string>

#include <string.h>
#include <fcntl.h> // for AT_FDCWD

class PathDescriptor {
	UniqueFileDescriptor fd;
	std::string path;

public:
	PathDescriptor(UniqueFileDescriptor &&_fd,
		       std::string_view _path) noexcept
		:fd(std::move(_fd)), path(_path) {}

	const std::string &GetPath() const noexcept {
		return path;
	}

	operator PathReference() const noexcept {
		return {fd, ""};
	}

	static int ToString(lua_State *L);
	static int Div(lua_State *L);
};

class RelativePath {
	Lua::Value l_base;
	const PathDescriptor &base;
	std::string path;

public:
	RelativePath(lua_State *L, Lua::StackIndex base_idx,
		     const PathDescriptor &_base,
		     std::string_view _path) noexcept
		:l_base(L, base_idx), base(_base), path(_path) {}

	operator PathReference() const noexcept {
		const PathReference b = base;
		return {b.directory_fd, path.c_str()};
	}

	std::string GetPath() const noexcept {
		return base.GetPath() + "/" + path;
	}

	static int ToString(lua_State *L);
	static int Div(lua_State *L);
};

static constexpr char lua_path_descriptor_class[] = "PathDescriptor";
using LuaPathDescriptor = Lua::Class<PathDescriptor,
				     lua_path_descriptor_class>;

static constexpr char lua_relative_path_class[] = "RelativePath";
using LuaRelativePath = Lua::Class<RelativePath,
				   lua_relative_path_class>;

static const auto &
CastLuaPathDescriptor(lua_State *L, int idx)
{
	return LuaPathDescriptor::Cast(L, idx);
}

static const auto *
CheckLuaPathDescriptor(lua_State *L, int idx)
{
	return LuaPathDescriptor::Check(L, idx);
}

static const auto &
CastLuaRelativePath(lua_State *L, int idx)
{
	return LuaRelativePath::Cast(L, idx);
}

static const auto *
CheckLuaRelativePath(lua_State *L, int idx)
{
	return LuaRelativePath::Check(L, idx);
}

int
PathDescriptor::ToString(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "Invalid parameters");

	const auto &pd = CastLuaPathDescriptor(L, 1);

	Lua::Push(L, pd.path);
	return 1;
}

int
PathDescriptor::Div(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "Invalid parameters");

	const auto &pd = CastLuaPathDescriptor(L, 1);

	if (!lua_isstring(L, 2))
		luaL_argerror(L, 2, "string expected");

	const char *s = lua_tostring(L, 2);

	LuaRelativePath::New(L, L, Lua::StackIndex{1}, pd, s);
	return 1;
}

int
RelativePath::ToString(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return luaL_error(L, "Invalid parameters");

	const auto &rp = CastLuaRelativePath(L, 1);
	Lua::Push(L, rp.GetPath());
	return 1;
}

int
RelativePath::Div(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "Invalid parameters");

	const auto &rp = CastLuaRelativePath(L, 1);

	if (!lua_isstring(L, 2))
		luaL_argerror(L, 2, "string expected");

	const char *s = lua_tostring(L, 2);

	LuaRelativePath::New(L, L, Lua::StackIndex{1},
			     rp.base, s);
	return 1;
}

void
RegisterLuaPath(lua_State *L) noexcept
{
	using namespace Lua;

	LuaPathDescriptor::Register(L);
	SetTable(L, RelativeStackIndex{-1},
		 "__tostring", PathDescriptor::ToString);
	SetTable(L, RelativeStackIndex{-1}, "__div", PathDescriptor::Div);
	lua_pop(L, 1);

	LuaRelativePath::Register(L);
	SetTable(L, RelativeStackIndex{-1},
		 "__tostring", RelativePath::ToString);
	SetTable(L, RelativeStackIndex{-1}, "__div", RelativePath::Div);
	lua_pop(L, 1);
}

void
NewLuaPathDescriptor(lua_State *L, UniqueFileDescriptor src,
		     std::string_view path)
{
	LuaPathDescriptor::New(L, std::move(src), path);
}

PathReference
GetLuaPath(lua_State *L, int idx)
{
	if (lua_isstring(L, idx)) {
		return {FileDescriptor{AT_FDCWD}, lua_tostring(L, idx)};
	} else if (auto *p = CheckLuaPathDescriptor(L, idx)) {
		return *p;
	} else if (auto *r = CheckLuaRelativePath(L, idx)) {
		return *r;
	} else {
		luaL_argerror(L, idx, "path expected");
		return {}; // unreachable
	}
}

std::string
GetLuaPathString(lua_State *L, int idx)
{
	if (lua_isstring(L, idx)) {
		return lua_tostring(L, idx);
	} else if (auto *p = CheckLuaPathDescriptor(L, idx)) {
		return p->GetPath();
	} else if (auto *r = CheckLuaRelativePath(L, idx)) {
		return r->GetPath();
	} else {
		luaL_argerror(L, idx, "path expected");
		return {}; // unreachable
	}
}
