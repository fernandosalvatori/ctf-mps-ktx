#!/usr/bin/env python3
"""Build the native Windows x64 CTF-MPS-KTX module without installing it."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "source" / "ktx"
ZIG_VERSION = "0.13.0"
TARGET = "x86_64-windows-gnu"


def arguments(description):
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("--zig", default=os.environ.get("ZIG_EXE", "zig"),
                        help="Zig executable (default: ZIG_EXE or zig on PATH)")
    parser.add_argument("--out-dir", type=Path, default=ROOT / "build",
                        help="output directory (default: repository/build)")
    return parser.parse_args()


def toolchain(executable):
    resolved = shutil.which(executable)
    if resolved is None:
        raise RuntimeError("Zig not found; pass --zig PATH or set ZIG_EXE.")
    version = subprocess.check_output([resolved, "version"], text=True).strip()
    if version != ZIG_VERSION:
        raise RuntimeError(f"Expected Zig {ZIG_VERSION}; found {version}.")
    return str(Path(resolved).resolve())


def environment(out):
    out.mkdir(parents=True, exist_ok=True)
    return dict(os.environ,
                ZIG_GLOBAL_CACHE_DIR=str(out / "zig-cache"),
                ZIG_LOCAL_CACHE_DIR=str(out / "zig-local"))


def relative(path):
    """Keep compiler source paths relative to the checkout where possible."""
    return Path(os.path.relpath(path, ROOT)).as_posix()


def write_json(path, data):
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_logged(command, env, log):
    with log.open("wb") as stream:
        result = subprocess.run(command, cwd=ROOT, env=env,
                                stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        print(log.read_text(encoding="utf-8", errors="replace")[-12000:])
    return result.returncode


def native_sources():
    cmake = (SOURCE / "CMakeLists.txt").read_text(encoding="utf-8")
    # bg_lib.c is the QVM libc replacement, excluded from native builds.
    names = list(dict.fromkeys(re.findall(r'"\$\{DIR_SRC\}/([^"\n]+\.c)"', cmake)))
    files = [SOURCE / "src" / name for name in names if name != "bg_lib.c"]
    if not files or any(not path.is_file() for path in files):
        raise RuntimeError("The native source list is empty or references missing files.")
    return files


def main():
    args = arguments(__doc__)
    zig = toolchain(args.zig)
    out = args.out_dir.resolve()
    env = environment(out)
    artifact = out / "qwprogs.dll"
    sources = native_sources()
    command = [zig, "cc", "-target", TARGET, "-mcpu=baseline", "-shared",
               "-O2", "-g", "-std=gnu17", "-fvisibility=hidden", "-DBOT_SUPPORT=1",
               "-I" + relative(SOURCE / "include")]
    command += [relative(path) for path in sources] + ["-o", relative(artifact)]
    write_json(out / "build-command.json", command)
    print(f"Building {len(sources)} C files with Zig {ZIG_VERSION} for {TARGET}...", flush=True)
    status = run_logged(command, env, out / "build.log")
    manifest = {"zig_version": ZIG_VERSION, "target": TARGET, "exit_code": status,
                "source_sha256": {relative(path): sha256(path) for path in
                    sorted(set(sources + list((SOURCE / "include").glob("*.h")) +
                               [SOURCE / "CMakeLists.txt"]))}}
    if status == 0:
        manifest["artifact"] = {"name": artifact.name, "sha256": sha256(artifact),
                                "bytes": artifact.stat().st_size}
        (out / "SHA256SUMS.txt").write_text(
            f"{manifest['artifact']['sha256']}  {artifact.name}\n", encoding="utf-8")
        print(f"Built {artifact}\nSHA-256 {manifest['artifact']['sha256']}")
    write_json(out / "build-result.json", manifest)
    return status


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"Build error: {error}", file=sys.stderr)
        sys.exit(1)
