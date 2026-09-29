#!/usr/bin/env python3
"""Compile and run all three module suites with deterministic engine doubles."""
import os
from pathlib import Path
import platform
import subprocess
import sys

from build import (ROOT, SOURCE, TARGET, ZIG_VERSION, arguments, environment,
                   relative, run_logged, sha256, toolchain, write_json)


def drone_hook_translation_unit(destination):
    """Include real modules once; upstream g_local.h has no include guard."""
    test = ROOT / "tests" / "test_drone_hook.c"
    text = test.read_text(encoding="utf-8")
    for name in ("drone", "hook"):
        path = SOURCE / "src" / f"ctfnormal_{name}.c"
        marker = f'#include "../source/ktx/src/ctfnormal_{name}.c"'
        if text.count(marker) != 1:
            raise RuntimeError(f"Missing or repeated module include: {marker}")
        source = path.read_text(encoding="utf-8").replace('#include "g_local.h"', "")
        text = text.replace(marker, source)
    destination.write_text(text, encoding="utf-8")
    return destination


def main():
    args = arguments(__doc__)
    if os.name != "nt" or platform.machine().lower() not in ("amd64", "x86_64"):
        raise RuntimeError("These executable suites require Windows x64.")
    zig = toolchain(args.zig)
    out = args.out_dir.resolve()
    env = environment(out)
    tests_out = out / "tests"
    tests_out.mkdir(parents=True, exist_ok=True)
    drone_unit = drone_hook_translation_unit(tests_out / "drone-hook-amalgam.c")
    suites = {
        "shrapnel": ([ROOT / "tests/shrapnel/test_shrapnel.c"], ["-DBOT_SUPPORT=1"]),
        "weld-burn": ([ROOT / "tests/weld-burn/harness.c",
                       SOURCE / "src/ctfnormal_weld.c",
                       SOURCE / "src/ctfnormal_burn.c"], []),
        "drone-hook": ([drone_unit], []),
    }
    results = {"zig_version": ZIG_VERSION, "target": TARGET,
               "scope": "Actual ported C with deterministic engine doubles; no server or rendered client.",
               "suites": {}}
    failed = False
    for name, (sources, defines) in suites.items():
        executable = tests_out / f"{name}.exe"
        command = [zig, "cc", "-target", TARGET, "-mcpu=baseline", "-std=gnu17",
                   "-O0", "-g", "-I" + relative(SOURCE / "include")]
        command += defines + [relative(path) for path in sources]
        command += ["-o", relative(executable)]
        write_json(tests_out / f"{name}-command.json", command)
        print(f"Building {name}...", flush=True)
        status = run_logged(command, env, tests_out / f"{name}-build.log")
        result = {"build_exit": status}
        if status == 0:
            tested = subprocess.run([str(executable)], cwd=ROOT, env=env,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            (tests_out / f"{name}-test.log").write_bytes(tested.stdout)
            result.update(test_exit=tested.returncode, executable_sha256=sha256(executable),
                          output=tested.stdout.decode("utf-8", errors="replace").strip())
            print(result["output"], flush=True)
            failed |= tested.returncode != 0
        else:
            failed = True
        results["suites"][name] = result
    results["passed"] = not failed
    write_json(tests_out / "results.json", results)
    print("All three suites passed." if not failed else "One or more suites failed.")
    return int(failed)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"Test error: {error}", file=sys.stderr)
        sys.exit(1)
