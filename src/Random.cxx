/*
 * Copyright 2017-2022 CM4all GmbH
 * All rights reserved.
 *
 * author: Babak Vahedipour <bk@cm4all.com>
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

#include "Random.hxx"

#include "lua/Class.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <random>
#include <stdexcept>
#include <string>

static std::random_device device;
static std::mt19937 prng;
static int seeded = 0;

class Random {

    std::string alphabet;
    mutable std::uniform_int_distribution<> d;

  public:
    Random(std::string_view _alphabet) noexcept
        : alphabet(_alphabet), d(0, _alphabet.size() - 1) {}

    static int New(lua_State *L);
    static int Make(lua_State *L);
};

static constexpr char lua_random_class[] = "Random";
using LuaRandom = Lua::Class<Random, lua_random_class>;

static const auto &CastLuaRandom(lua_State *L, int idx) {

    return LuaRandom::Cast(L, idx);
}

int Random::New(lua_State *L) {

    if (lua_gettop(L) != 2)
        return luaL_error(L, "Invalid parameters");
    if (!lua_isstring(L, 2))
        luaL_argerror(L, 2, "string expected");
    size_t length;
    const char *alphabet = lua_tolstring(L, 2, &length);
    if (length < 2)
        luaL_argerror(L, 2, "alphabet string too short");
    if (!seeded) {
        seeded = 1;
        prng.seed(device());
    }
    LuaRandom::New(L, alphabet);
    return 1;
}

int Random::Make(lua_State *L) {

    if (lua_gettop(L) != 2)
        return luaL_error(L, "Invalid parameters");
    if (!lua_isnumber(L, 2))
        luaL_argerror(L, 2, "integer expected");
    auto length = lua_tointeger(L, 2);
    if (length < 1 || length >= 256)
        luaL_argerror(L, 2, "argument must be > 0 && < 256 ");
    const auto &rd = CastLuaRandom(L, 1);
    char buffer[256];
    for (lua_Integer i = 0; i < length; i++)
        buffer[i] = rd.alphabet[rd.d(prng)];
    buffer[length] = 0;
    Lua::Push(L, buffer);
    return 1;
}

void RegisterLuaRandom(lua_State *L) {
    using namespace Lua;

    LuaRandom::Register(L);

    lua_newtable(L);
    SetTable(L, RelativeStackIndex{-1}, "make", Random::Make);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    lua_newtable(L);
    SetTable(L, RelativeStackIndex{-1}, "new", Random::New);
    lua_setglobal(L, "Random");
}
