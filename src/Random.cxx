// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

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
