// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "CommandLine.hxx"

CommandLine ParseCommandLine(int argc, char **argv) {
    if (argc < 3 || argc > 4)
        throw "Usage: cm4all-commence SCRIPT_PATH DESTINATION_PATH [ARGS.json]";

    CommandLine cmdline;
    cmdline.script_path = argv[1];
    cmdline.destination_path = argv[2];
    if (argc > 3)
        cmdline.args_json_path = argv[3];
    return cmdline;
}
