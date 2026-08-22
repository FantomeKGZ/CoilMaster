Import("env")

import os
import subprocess
from datetime import datetime, timezone


def git_value(*args):
    try:
        return subprocess.check_output(
            ["git", *args], stderr=subprocess.DEVNULL, text=True
        ).strip()
    except Exception:
        return "unknown"


def c_string(value):
    return value.replace("\\", "\\\\").replace('"', '\\"')


sha = git_value("rev-parse", "--short=12", "HEAD")
dirty = git_value("status", "--porcelain")
if dirty and dirty != "unknown":
    sha += "-dirty"
branch = git_value("rev-parse", "--abbrev-ref", "HEAD")
built_utc = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

# Do not pass string metadata through CPPDEFINES: SCons/compiler command-line
# quoting can strip the C-string quotes and reinterpret SHA/branch/time as C++
# tokens. Generate a private build header and force-include it instead.
build_dir = env.subst("$BUILD_DIR")
os.makedirs(build_dir, exist_ok=True)
header_name = "CM_BuildIdentityGenerated.h"
header_path = os.path.join(build_dir, header_name)
with open(header_path, "w", encoding="utf-8", newline="\n") as header:
    header.write("#pragma once\n")
    header.write('#define CM_FIRMWARE_GIT_SHA "{}"\n'.format(c_string(sha)))
    header.write('#define CM_FIRMWARE_GIT_BRANCH "{}"\n'.format(c_string(branch)))
    header.write('#define CM_FIRMWARE_BUILD_UTC "{}"\n'.format(c_string(built_utc)))

env.Append(CPPPATH=[build_dir])
env.Append(CCFLAGS=["-include", header_name])
