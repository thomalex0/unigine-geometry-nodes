#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import platform


_platform = platform.platform().lower()
_cmake_gen = ""
is_linux = False
is_windows = False
if _platform.startswith("linux"):
    _platform = "linux"
    is_linux = True
elif _platform.startswith("windows"):
    _platform = "windows"
    _cmake_gen = " -G \"Visual Studio 15 2017 Win64\""
    is_windows = True
else:
    sys.exit("Unknown platform")



build_commands = [
    {
        "name": "cmake configure release",
        "command": "cmake -H. -Bjunk/release_" + _platform + " " + _cmake_gen + " -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DCMAKE_INSTALL_PREFIX=../bin",
    },
    {
        "name": "cmake build release",
        "command": 'cmake --build junk/release_' + _platform + ' --parallel 8 --config Release',
    }, 
    {
        "name": "cmake configure debug",
        "command": "cmake -H. -Bjunk/debug_" + _platform + " " + _cmake_gen + " -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DCMAKE_INSTALL_PREFIX=../bin",
    },
    {
        "name": "cmake build debug",
        "command": "cmake --build junk/debug_" + _platform + " --parallel 8 --config RelWithDebInfo",
    }, 
]

os.chdir("source/GeometryNodesPlugin")

for _cmd in build_commands:
    print("Starting... " + _cmd["name"])
    ret = os.system(_cmd["command"])
    if ret != 0:
        print("Failed... " + _cmd["name"])
        sys.exit(-1)
    print("Done. " + _cmd["name"] + "\n\n-----\n\n")
input()