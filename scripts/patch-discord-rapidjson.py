#!/usr/bin/env python3
"""FetchContent PATCH_COMMAND for discord-rpc (see CMakeLists.txt).

The bundled rapidjson-1.1.0 declares an ill-formed GenericStringRef
operator= (it assigns to a const member); clang 22+ rejects it outright.
Upstream rapidjson removed the operator (rapidjson#1364) — drop the line
the same way. Runs from the discord_rpc source dir; idempotent.
"""

from pathlib import Path

BAD = "GenericStringRef& operator=(const GenericStringRef& rhs)"
path = Path("thirdparty/rapidjson-1.1.0/include/rapidjson/document.h")
if not path.is_file():
    # discord-rpc downloads rapidjson during its own configure step, so at
    # FetchContent patch time the file may not exist yet — the post-
    # MakeAvailable execute_process invocation catches it then.
    print("patch-discord-rapidjson: rapidjson not present yet, nothing to do")
    raise SystemExit(0)
lines = path.read_text().splitlines(keepends=True)
kept = [l for l in lines if BAD not in l]
if len(kept) != len(lines):
    path.write_text("".join(kept))
    print(f"patch-discord-rapidjson: removed {len(lines) - len(kept)} line(s)")
else:
    print("patch-discord-rapidjson: already patched")
