// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#pragma once

#include <string>

struct CommandLine {
    const char *script_path = nullptr;

    const char *destination_path = nullptr;

    const char *args_json_path = nullptr;
};

CommandLine ParseCommandLine(int argc, char **argv);
