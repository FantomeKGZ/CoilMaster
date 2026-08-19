Import("env")

import subprocess
from datetime import datetime, timezone


def git_value(*args):
    try:
        return subprocess.check_output(
            ["git", *args], stderr=subprocess.DEVNULL, text=True
        ).strip()
    except Exception:
        return "unknown"


sha = git_value("rev-parse", "--short=12", "HEAD")
dirty = git_value("status", "--porcelain")
if dirty and dirty != "unknown":
    sha += "-dirty"
branch = git_value("rev-parse", "--abbrev-ref", "HEAD")
built_utc = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

for name, value in (
    ("CM_FIRMWARE_GIT_SHA", sha),
    ("CM_FIRMWARE_GIT_BRANCH", branch),
    ("CM_FIRMWARE_BUILD_UTC", built_utc),
):
    escaped = value.replace('\\', '\\\\').replace('"', '\\"')
    env.Append(CPPDEFINES=[(name, '\"' + escaped + '\"')])
