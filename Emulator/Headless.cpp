// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// This FILE is dual-licensed. You are free to choose between:
//
//     - The GNU General Public License v3 (or any later version)
//     - The Mozilla Public License v2
//
// SPDX-License-Identifier: GPL-3.0-or-later OR MPL-2.0
// -----------------------------------------------------------------------------
/// @file

#include "config.h"
#include "Headless.h"
#include "C64.h"
#include "Script.h"
#include "IOUtils.h"
#include <chrono>
#include <iostream>

#ifndef _WIN32
#include <getopt.h>
#endif

int main(int argc, char *argv[])
{
    try {

        return vc64::Headless().main(argc, argv);

    } catch (vc64::SyntaxError &e) {

        std::cout << "Usage: VirtualC64Core [-svm] | { [-vm] <script> } " << std::endl;
        std::cout << std::endl;
        std::cout << "       -c or --check     Checks the integrity of the build" << std::endl;
        std::cout << "       -s or --size      Reports the size of certain objects" << std::endl;
        std::cout << "       -v or --verbose   Print executed script lines" << std::endl;
        std::cout << "       -m or --messages  Observe the message queue" << std::endl;
        std::cout << std::endl;

        if (auto what = string(e.what()); !what.empty()) {
            std::cout << what << std::endl;
        }
        
        return 1;

    } catch (vc64::Error &e) {

        std::cout << "Error: " << e.what() << std::endl;
        return 1;
        
    } catch (std::exception &e) {

        std::cout << "System Error: " << e.what() << std::endl;
        return 1;
    
    } catch (...) {
    
        std::cout << "Error" << std::endl;
    }
    
    return 0;
}

namespace vc64 {

int
Headless::main(int argc, char *argv[])
{
    std::cout << "VirtualC64 Headless v" << VirtualC64::version();
    std::cout << " - (C)opyright Dirk W. Hoffmann" << std::endl << std::endl;

    // Parse all command line arguments
    parseArguments(argc, argv);

    // Check for the --size option
    if (keys.find("size") != keys.end()) {

        reportSize();
        return 0;

    } else {

        return execScript();
    }
}

#ifdef _WIN32

void
Headless::parseArguments(int argc, char *argv[])
{
    keys["check"] = "1";
    keys["size"] = "1";
    keys["verbose"] = "1";
    keys["arg1"] = testScriptPath().string();
}

#else

void
Headless::parseArguments(int argc, char *argv[])
{
    static struct option long_options[] = {
        
        { "check",      no_argument,    NULL,   'c' },
        { "size",       no_argument,    NULL,   's' },
        { "verbose",    no_argument,    NULL,   'v' },
        { "messages",   no_argument,    NULL,   'm' },
        { NULL,         0,              NULL,    0  }
    };
    
    // Don't print the default error messages
    opterr = 0;
    
    // Remember the execution path
    keys["exec"] = fs::absolute(argv[0]);

    // Parse all options
    while (1) {

        int arg = getopt_long(argc, argv, ":csvm", long_options, NULL);
        if (arg == -1) break;

        switch (arg) {

            case 'c':
                keys["check"] = "1";
                break;

            case 's':
                keys["size"] = "1";
                break;

            case 'v':
                keys["verbose"] = "1";
                break;

            case 'm':
                keys["messages"] = "1";
                break;

            case ':':
                throw SyntaxError("Missing argument for option '" +
                                  string(argv[optind - 1]) + "'");

            default:
                throw SyntaxError("Invalid option '" +
                                  string(argv[optind - 1]) + "'");
        }
    }
    
    // Parse all remaining arguments
    auto nr = 1;
    while (optind < argc) {
        keys["arg" + std::to_string(nr++)] = fs::absolute(fs::path(argv[optind++])).string();
    }

    // Check for syntax errors
    checkArguments();

    // Create the selftest script if needed
    if (keys.find("check") != keys.end()) keys["arg1"] = testScriptPath().string();
}

#endif

void
Headless::checkArguments()
{
    if (keys.find("check") != keys.end() || keys.find("size") != keys.end()) {

        // No input file must be given
        if (keys.find("arg1") != keys.end()) {
            throw SyntaxError("No script file must be given");
        }

    } else {

        // The user needs to specify a single input file
        if (keys.find("arg1") == keys.end()) {
            throw SyntaxError("No script file is given");
        }
        if (keys.find("arg2") != keys.end()) {
            throw SyntaxError("More than one script file is given");
        }

        // The input file must exist
        if (!util::fileExists(keys["arg1"])) {
            throw SyntaxError("File " + keys["arg1"] + " does not exist");
        }
    }
}

std::filesystem::path
Headless::testScriptPath()
{
    auto path = std::filesystem::temp_directory_path() / "selftest.ini";
    auto file = std::ofstream(path);

    for (isize i = 0; i < isizeof(testScript) / isizeof(const char *); i++) {
        file << testScript[i] << std::endl;
    }
    return path;
}

void
process(const void *listener, Message msg)
{
    ((Headless *)listener)->process(msg);
}

void
Headless::process(Message msg)
{
    static bool messages = keys.find("messages") != keys.end();
    
    if (messages) {
        
        std::cout << MsgTypeEnum::key(msg.type);
        std::cout << "(" << msg.value << ")";
        std::cout << std::endl;
    }

    switch (msg.type) {

        case MSG_RSH_EXEC:

            returnCode = 0;
            barrier.unlock();
            break;

        case MSG_RSH_ERROR:
        case MSG_ABORT:

            returnCode = 1;
            barrier.unlock();
            break;

        default:
            break;
    }
}

void
Headless::reportSize()
{
    msg("               C64 : %zu bytes\n", sizeof(C64));
    msg("         C64Memory : %zu bytes\n", sizeof(C64Memory));
    msg("       DriveMemory : %zu bytes\n", sizeof(DriveMemory));
    msg("               CPU : %zu bytes\n", sizeof(CPU));
    msg("               CIA : %zu bytes\n", sizeof(CIA));
    msg("             VICII : %zu bytes\n", sizeof(VICII));
    msg("         SIDBridge : %zu bytes\n", sizeof(SIDBridge));
    msg("         PowerPort : %zu bytes\n", sizeof(PowerPort));
    msg("       ControlPort : %zu bytes\n", sizeof(ControlPort));
    msg("     ExpansionPort : %zu bytes\n", sizeof(ExpansionPort));
    msg("        SerialPort : %zu bytes\n", sizeof(SerialPort));
    msg("          Keyboard : %zu bytes\n", sizeof(Keyboard));
    msg("             Drive : %zu bytes\n", sizeof(Drive));
    msg("          ParCable : %zu bytes\n", sizeof(ParCable));
    msg("         Datasette : %zu bytes\n", sizeof(Datasette));
    msg("        RetroShell : %zu bytes\n", sizeof(RetroShell));
    msg("  RegressionTester : %zu bytes\n", sizeof(RegressionTester));
    msg("          Recorder : %zu bytes\n", sizeof(Recorder));
    msg("          MsgQueue : %zu bytes\n", sizeof(MsgQueue));
    msg("          CmdQueue : %zu bytes\n", sizeof(CmdQueue));
    msg("\n");
}

int
Headless::execScript()
{
    // Create an emulator instance
    VirtualC64 c64;

    // Redirect shell output to the console in verbose mode
    if (keys.find("verbose") != keys.end()) c64.retroShell.setStream(std::cout);

    // Read the input script
    auto script = MediaFile::make(keys["arg1"], FILETYPE_SCRIPT);

    // Launch the emulator thread
    c64.launch(this, vc64::process);

    // Execute the script
    barrier.lock();
    c64.retroShell.execScript(*script);
    barrier.lock();

    return returnCode;
}

}
