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

#include "PwHash.hxx"
#include "lua/Error.hxx"
#include "lua/Util.hxx"
#include "system/Error.hxx"
#include "system/Urandom.hxx"

extern "C" {
#include <lauxlib.h>
}

#include <sodium/crypto_pwhash.h>

#ifdef HAVE_LIBCRYPT
#include <crypt.h>
#endif

#include <array>
#include <stdexcept>

using std::string_view_literals::operator""sv;

namespace Lua {

static std::string_view CheckString(lua_State *L, int idx) {
    size_t length;
    const char *data = luaL_checklstring(L, idx, &length);
    return {data, length};
}

static std::string_view OptString(lua_State *L, int idx,
                                  const char *d = nullptr) {
    size_t length;
    const char *data = luaL_optlstring(L, idx, d, &length);
    return {data, length};
}

struct PwHashDefault {
    static constexpr std::size_t STRBYTES = crypto_pwhash_STRBYTES;
    static constexpr unsigned long long OPSLIMIT_INTERACTIVE =
        crypto_pwhash_OPSLIMIT_INTERACTIVE;
    static constexpr size_t MEMLIMIT_INTERACTIVE =
        crypto_pwhash_MEMLIMIT_INTERACTIVE;

    static int str(char out[STRBYTES], const char *passwd,
                   unsigned long long passwdlen, unsigned long long opslimit,
                   size_t memlimit) noexcept {
        return crypto_pwhash_str(out, passwd, passwdlen, opslimit, memlimit);
    }
};

struct PwHashArgon2i {
    static constexpr std::size_t STRBYTES = crypto_pwhash_argon2i_STRBYTES;
    static constexpr unsigned long long OPSLIMIT_INTERACTIVE =
        crypto_pwhash_argon2i_OPSLIMIT_INTERACTIVE;
    static constexpr size_t MEMLIMIT_INTERACTIVE =
        crypto_pwhash_argon2i_MEMLIMIT_INTERACTIVE;

    static int str(char out[STRBYTES], const char *passwd,
                   unsigned long long passwdlen, unsigned long long opslimit,
                   size_t memlimit) noexcept {
        return crypto_pwhash_argon2i_str(out, passwd, passwdlen, opslimit,
                                         memlimit);
    }
};

struct PwHashArgon2id {
    static constexpr std::size_t STRBYTES = crypto_pwhash_argon2id_STRBYTES;
    static constexpr unsigned long long OPSLIMIT_INTERACTIVE =
        crypto_pwhash_argon2id_OPSLIMIT_INTERACTIVE;
    static constexpr size_t MEMLIMIT_INTERACTIVE =
        crypto_pwhash_argon2id_MEMLIMIT_INTERACTIVE;

    static int str(char out[STRBYTES], const char *passwd,
                   unsigned long long passwdlen, unsigned long long opslimit,
                   size_t memlimit) noexcept {
        return crypto_pwhash_argon2id_str(out, passwd, passwdlen, opslimit,
                                          memlimit);
    }
};

template <typename PwHash>
static int l_sodium_pwhash(lua_State *L, std::string_view password) {
    char hash[PwHash::STRBYTES];
    if (PwHash::str(hash, password.data(), password.size(),
                    PwHash::OPSLIMIT_INTERACTIVE,
                    PwHash::MEMLIMIT_INTERACTIVE) != 0)
        throw std::runtime_error("crypto_pwhash_str() failed");

    lua_pushstring(L, hash);
    return 1;
}

#ifdef HAVE_LIBCRYPT

/**
 * Generate a SHA-512 salt for crypt_r() and return it as a
 * null-terminated std::array<char>.
 */
static auto GenerateCryptSalt() {
    static constexpr std::size_t SALT_CHARS = 16;

    std::array<uint8_t, SALT_CHARS> r;
    UrandomFill(std::as_writable_bytes(std::span(r)));

    std::array<char, 3 + SALT_CHARS + 1> salt;
    char *p = salt.data();
    *p++ = '$';
    *p++ = '6';
    *p++ = '$';

    static constexpr std::string_view alphabet =
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"sv;
    static_assert(alphabet.size() == 64);
    for (std::size_t i = 0; i < SALT_CHARS; ++i)
        *p++ = alphabet[r[i] % alphabet.size()];

    *p = 0;

    return salt;
}

static int l_sha512_pwhash(lua_State *L, const char *password) {
    struct crypt_data crypt_data{};
    const auto salt = GenerateCryptSalt();
    const char *hash = crypt_r(password, salt.data(), &crypt_data);
    if (hash == nullptr)
        throw MakeErrno("crypt_r() failed");

    lua_pushstring(L, hash);
    return 1;
}

#endif // HAVE_LIBCRYPT

static int l_pwhash(lua_State *L) {
    const auto password = CheckString(L, 1);
    const auto setting = OptString(L, 2);

    if (lua_gettop(L) > 2)
        return luaL_error(L, "Too many parameters");

    try {
        if (setting.empty())
            return l_sodium_pwhash<PwHashDefault>(L, password);
        else if (setting == crypto_pwhash_argon2i_STRPREFIX)
            return l_sodium_pwhash<PwHashArgon2i>(L, password);
        else if (setting == crypto_pwhash_argon2id_STRPREFIX)
            return l_sodium_pwhash<PwHashArgon2id>(L, password);
#ifdef HAVE_LIBCRYPT
        else if (setting == "$6$"sv)
            return l_sha512_pwhash(L, password.data());
#endif
        else {
            luaL_argerror(L, 2, "Unrecognized setting");
            abort();
        }
    } catch (...) {
        Lua::RaiseCurrent(L);
    }
}

void RegisterPwHash(lua_State *L) noexcept {
    Lua::SetGlobal(L, "pwhash", l_pwhash);
}

} // namespace Lua
