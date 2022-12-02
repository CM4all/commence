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
