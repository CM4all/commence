// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "Template.hxx"
#include "lua/Assert.hxx"
#include "lua/Error.hxx"
#include "util/ScopeExit.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <stdexcept>
#include <string>

using std::string_view_literals::operator""sv;

void RunTemplate(lua_State *L, std::string_view t,
                 TemplateWriteCallback callback) {
    while (!t.empty()) {
        const Lua::ScopeCheckStack check_stack{L};

        auto i = t.find("{{"sv);
        if (i == t.npos) {
            callback(t);
            break;
        }

        if (i > 0) {
            callback(t.substr(0, i));
            t = t.substr(i + 2);
        }

        i = t.find("}}"sv);
        if (i == t.npos)
            throw std::invalid_argument{"Missing '}}'"};

        const auto e = t.substr(0, i);
        t = t.substr(i + 2);

        auto e2 = std::string{"return "};
        e2 += e;

        if (luaL_loadbuffer(L, e2.data(), e2.size(), "dummy") != 0 ||
            lua_pcall(L, 0, 1, 0) != 0)
            throw Lua::PopError(L);
        AtScopeExit(L) { lua_pop(L, 1); };

        if (luaL_callmeta(L, -1, "__tostring")) {
            AtScopeExit(L) { lua_pop(L, 1); };
            callback({lua_tostring(L, -1), lua_strlen(L, -1)});
        } else {
            size_t length;
            const char *s = lua_tolstring(L, -1, &length);
            callback({s, length});
        }
    }
}
