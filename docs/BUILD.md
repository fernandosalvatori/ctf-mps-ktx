English | [Português (Brasil)](BUILD.pt-BR.md)

# Build and test CTF-MPS-KTX

The validated workflow produces `build/qwprogs.dll`, a native Windows x64 module for MVDSV. It requires Python 3.9 or later and **Zig 0.13.0**. No additional Python packages, CMake, Visual Studio, commercial Quake files, or running server are required.

Get the compiler from the [official Zig 0.13.0 download](https://ziglang.org/download/0.13.0/zig-windows-x86_64-0.13.0.zip). The Windows x64 ZIP has SHA-256 `d859994725ef9402381e557c60bb57497215682e355204d754ee3df75ee3c158`, as listed in the [official index](https://ziglang.org/download/index.json). Extract the ZIP and add the directory containing `zig.exe` to your `PATH`.

From the repository root, run:

```powershell
python scripts/build.py --zig zig
python scripts/test.py --zig zig
```

If the compiler is outside your `PATH`, pass its executable path to `--zig`, quoting it if it contains spaces, or set the `ZIG_EXE` environment variable. The scripts check the compiler version before building. They locate the source files relative to their own location, so the checkout directory can have any name. Use `--out-dir` to choose another output directory; relative paths for this option are resolved from the terminal's working directory.

## Build

`scripts/build.py` reads the C files listed in `source/ktx/CMakeLists.txt` and excludes `bg_lib.c`, which belongs to the QVM build. The target is `x86_64-windows-gnu`, with the `baseline` CPU, `-O2` optimization, `-g` debug symbols, and the `gnu17` dialect.

The KTX 1.47 build uses `BOT_SUPPORT=1` because that version retains internal references to Frogbot symbols. This does not enable bots in the server configuration or include the earlier CTF bot experiment. The test module in `tests/runtime.c` and the `CFN_TEST` macro are not included in this DLL.

Outputs:

- `build/qwprogs.dll`: the compiled module.
- `build/SHA256SUMS.txt`: the SHA-256 hash of the resulting DLL.
- `build/build-result.json`: compiler version, target, result, DLL hash, and hashes of source files and headers.
- `build/build-command.json` and `build/build.log`: the build command and compiler diagnostics.
- `build/zig-cache` and `build/zig-local`: local compiler caches.

The hashes identify each build; debug symbols, the checkout path, and linker metadata can change the hash between machines. Byte-for-byte reproducibility is not claimed.

## Tests

`scripts/test.py` compiles and runs three suites on Windows x64:

| Suite | Code exercised |
| --- | --- |
| Shrapnel | `ctfnormal_shrapnel.c`: firing, impact, fragments, damage, lifetime, and ownership |
| Weld/Burn | `ctfnormal_weld.c` and `ctfnormal_burn.c`: projectile, damage, ignition, layers, and cleanup |
| Drone/Hook | `ctfnormal_drone.c` and `ctfnormal_hook.c`: targeting, guidance, collisions, and grappling hook |

The suites compile the actual implementations, replacing only the KTX/engine boundary with deterministic functions. The Drone/Hook suite generates a translation unit in `build/tests/` so that the upstream header, which has no include guard, is included only once. It uses the current contents of both modules without maintaining a separate copy of their logic.

Executables, commands, logs, and the `results.json` summary are written to `build/tests/`. A compilation failure or failed check returns a nonzero exit code. Neither script starts MVDSV, connects players, installs files on a server, or changes server configuration.

These tests do not replace a real match: they do not verify rendering, client prediction, networking, engine collisions, or gameplay balance. `tests/runtime.c` contains checks for a separate instrumented build, but is not part of this command or the distributed DLL.

## Continuous integration

`.github/workflows/windows.yml` runs the same two commands on Windows after downloading and verifying the official Zig ZIP. The workflow uploads the DLL and logs as run artifacts. It does not publish releases or deploy a server. The GitHub Actions history shows whether a remote run actually passed; the presence of a workflow file alone does not establish that it has run.
