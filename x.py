#!/usr/bin/env python3

import os
import sys
import imp
import subprocess

app_dir = os.path.dirname(os.path.abspath(__file__))

if len(sys.argv) == 1 \
and os.path.basename(os.path.abspath(sys.argv[0])) == os.path.basename(os.path.abspath(__file__)):
    for args in (["clean"], ["all"]):
        subprocess.call([
            "env",
            "BOLOS_SDK=" + os.path.join("sdk", "nanos-secure-sdk"),
            "make"
        ] + args, cwd=app_dir)
    subprocess.call([
        sys.executable,
        os.path.join(app_dir, "sdk", "speculos", "speculos.py"),
        "--debug",
        "-m", "nanos",
        os.path.join(app_dir, "bin", "app.elf"),
        "--display", "headless",
        "--vnc-port", os.environ["VNC_PORT"],
        "--apdu-port", os.environ["ADPU_PORT"]
    ])
