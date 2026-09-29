# CTF MPS KTX

English | [Português (Brasil)](README.pt-BR.md)

A port of the **CTFNormal / ServerModules** special weapons to **QuakeWorld**, using **KTX 1.47** and **MVDSV 1.11**. The module identifies itself as `1.47-ctfnormal.1` and adds **Drone, Shrapnel and WeldGun**, along with **Burn, the Lightning modifications and the original grappling hook**.

**Creator and maintainer of this port:** Fernando (Droni) Salvatori — [@fernandosalvatori](https://github.com/fernandosalvatori). **CTF MPS KTX was first published on 28 September 2026.** This credit identifies the creator and maintainer of the port; the existing authorship of KTX, MVDSV, ServerModules and other components is preserved.

CTF gameplay, scoring, team management and the protocol remain under KTX/MVDSV. The example configuration does not populate matches with bots and keeps runes disabled. This repository contains code, tests, documentation and example configuration files; **it does not distribute PAKs, maps, models, sounds or the engine executable**.

This adaptation is under development. It has automated module tests and engine tests, but **full gameplay equivalence to the original NetQuake mod has not been demonstrated, and a complete visual playtest with a human player has not been completed**.

## Features and controls

The numbers below are the standard weapon selection impulses. The corresponding keys depend on the client's bindings.

| Feature | Command | Behavior |
|---|---|---|
| WeldGun | `impulse 4` | Selecting the nailgun activates WeldGun; repeat to toggle between WeldGun and regular nails. Uses 1 nail per shot. |
| Drone | `impulse 6` again while the GL is selected | Toggles between grenade and Drone modes. Uses 1 rocket; keeps up to four drones per player. |
| Shrapnel | `impulse 7` again while the RL is selected | Toggles between rocket and Shrapnel modes. Uses 1 rocket and releases incendiary fragments. |
| Lightning | `impulse 8` | Lightning gun with the original module's sounds and underwater discharge. |
| Grappling hook | `+hook` / `-hook` | Hold to launch and keep pulling; release to detach. Original impulses: 98/97. |
| Toggle hook | `impulse 22` | Additional shortcut to launch or release the same hook. |
| Burn | Automatic | Fire with up to three layers, spread to nearby players, extinguishing in water and death effects. |

Optional example for the client console:

```text
alias +hook "impulse 98"
alias -hook "impulse 97"
bind mouse3 +hook
```

The server provides the aliases without replacing keyboard bindings. It also provides `help-drone`, `help-shrapnel`, `help-weldgun` and `help-hook`, corresponding to impulses 215, 216, 217 and 219.

### Weapon behavior

- **Drone:** a homing projectile with its own health, target selection and updates, speed adaptation, ricochets and explosion. Launching a fifth drone schedules the oldest one to explode. The algorithm was ported from ServerModules; it is not derived from player-bot AI.
- **Shrapnel:** a missile with an accompanying flame and three or four fragments, depending on the impact and the original random selection. Fragments bounce, deal radius damage and can ignite targets.
- **WeldGun:** fast projectiles of glowing metal, with radius damage and a chance to start Burn.
- **Burn:** up to three fire layers, each with its own timer. Deals periodic damage and can spread to nearby players. Water above waist level extinguishes the fire on the next damage tick.
- **Lightning:** underwater discharge requires full submersion, consumes cells and caps its base strength at 400. Sounds respond to movement of the hit point and to the original timers.
- **Grappling hook:** an eight-link chain, attachment to surfaces or entities, contact damage, pulling and swinging from ServerModules Hook 1.2. The native KTX hook remains disabled to avoid overlapping hook systems.

## Repository layout

```text
source/ktx/          KTX 1.47 with the C port
tests/              Module tests and integration fixture
scripts/            Build, test and local installation preparation
server-example/ktx/ Example configuration without credentials
docs/CHANGES.md     Detailed port notes, adaptations and validation
LICENSE.md          GNU GPL version 2 text
LICENSES/NOTICE.md  Origins, authorship and license notices
```

The original module code was converted from QuakeC to native C. It does not run a second `progs.dat` inside KTX. The MVDSV executable is a separate dependency.

## Build and test

The build path used for this port is **Windows x64**, **Python 3.9 or later** and **Zig 0.13.0**. Other targets supported by upstream KTX have not necessarily been validated with these modifications.

Make `zig` available on `PATH`, or replace the `--zig` argument with the path to the executable. From the repository root:

```text
python scripts/build.py --zig zig
python scripts/test.py --zig zig
```

The build produces `build/qwprogs.dll`; the tests write their summary to `build/tests/results.json`. Both scripts accept `ZIG_EXE` and `--out-dir`; see [docs/BUILD.md](docs/BUILD.md).

The unit tests use the actual C files and replace the engine boundary with deterministic responses. The integration fixture is separate and must only be included in a test build; it is not an administrative command in the normal build.

## Prepare a local installation

You must supply files from Quake and CTFNormal installations you are entitled to use, a separately obtained MVDSV 1.11 executable, and the KTX 1.47 resources. The preparation script **imports local files**; it does not download game data or the engine.

The directory passed to `--quake-dir` must contain `id1/pak0.pak` and `id1/pak1.pak`. `--ctfnormal-dir` points to the reference mod and its `Maps` subdirectory. `--ktx-assets` points to `resources/example-configs/ktx` in a KTX 1.47 distribution containing the required models and sounds.

Example with relative paths that you must replace with your own:

```text
python scripts/prepare_server.py --quake-dir ../quake --ctfnormal-dir ../ctfnormal --mvdsv ../mvdsv/mvdsv.exe --ktx-assets ../ktx-1.47/resources/example-configs/ktx --output ../ctf-mps-runtime
```

Use a **new output directory**. Keep the source installation separate for comparison. Preparation uses `build/qwprogs.dll` by default; `--progs` lets you supply another compiled module. Run `python scripts/prepare_server.py --help` for the other preparation checks.

After preparation, use `Iniciar.cmd` (start), `Status.cmd` (status) and `Parar.cmd` (stop). They operate only on the instance registered by the launcher itself, checking the executable and process start time before stopping it; they do not require RCON. You can also start MVDSV directly from the runtime directory with the compiled KTX module:

```text
mvdsv.exe -basedir . -game ktx -port 27561 +exec server.cfg +map e1m1
```

Use a **QuakeWorld** client. To connect to a server on the same computer:

```text
disconnect
spectator 0
team blue
connect localhost:27561
```

Replace `blue` with `red` to choose the other team. The NetQuake mod's `tblue` and `tred` commands are not KTX join commands. The example configuration does not create a service, a startup task or a router port-forwarding rule.

## Configuration

| Option | Example | Purpose |
|---|---|---|
| `k_ctfnormal` | `1` | Enable the extension |
| `k_mode` | `4` | CTF |
| `k_matchless` | `1` | Continuous matches |
| `teamplay` | `4` | Protect teammates' health and armor while retaining self-damage |
| `fraglimit` / `timelimit` | `150` / `40` | Reference configuration limits; configurable |
| `sv_maxspeed` / `sv_accelerate` | `350` / `20` | Reference configuration values |
| `k_fb_enabled` | `0` | Disable bots |
| `k_ctf_runes` | `0` | Disable runes |
| `k_ctf_hook` | `0` | Disable the native KTX hook and use the ported hook |
| Runtime port | UDP `27561` | Example instance |

The 150/40 limits, hostname, port and rotation are configuration preferences, not weapon requirements. The old ServerModules `teamplay` module bitmask cannot be copied directly into a KTX team rule.

Mode configuration must restore `sv_loadentfiles 1` and `sv_loadentfiles_dir ctf`, because KTX preset initialization may clear that directory. The example files keep CTF settings consistent with the extension. Private administration settings belong in the local installation, outside the repository.

## Known differences and limitations

**Physics and protocol:** movement, client prediction, aiming and simulation use QuakeWorld/MVDSV. Preserving weapon constants does not make the experience identical to NetQuake. The old engine's timing was not copied literally.

**HUD:** ServerModules used key bits to indicate the alternative weapon and Drone threats. These bits conflict with flag display in QuakeWorld CTF. The port stores those states separately and uses mode and warning messages; it does not promise the original client's icon.

**Scope:** administration, rankings, menus and item randomization from the ServerModules package were not fully ported. Native KTX CTF continues to manage matches. The optional spawn-protection module was disabled in the reference configuration and was not added.

**Data:** the code and configuration do not replace the required map, model and sound resources. This project does not implicitly authorize redistribution of commercial Quake data.

## Recorded validation

| Suite or check | Result |
|---|---|
| WeldGun/Burn | 3324 checks against the actual C modules |
| Shrapnel | 252 checks against the actual C modules |
| Drone/Hook | 98 checks against the actual C modules |
| Unit-test total | **3674 checks** |
| Integrated MVDSV/KTX | **27 checks**, zero failures |
| Maps | **20 maps** loaded with correct status, active CTF and both flags |

The **3674 unit checks were rerun against this public source tree**, on Windows x64 with Zig 0.13.0, with no failures. The DLL was also built and loaded, and its `vmMain` and `dllEntry` exports were verified. The **27 engine checks and 20 maps** are earlier results from the same CFN1 state; they are not run by the unit-test command above. Integration checks exercised selection, ammunition, projectiles, the drone queue, real damage, Burn/water/death/respawn, teammate protection, self-damage, underwater Lightning and hook impulses. The map scan uses external data not distributed here.

**A complete visual playtest with human players is still pending**, including matches, flag captures, hook feel, collisions, audio and network behavior. Loading 20 maps is not equivalent to testing 20 matches.

See [docs/CHANGES.md](docs/CHANGES.md) for formulas, integration points, deliberate differences and test limitations.

## Origins and license

Based on [KTX](https://github.com/QW-Group/ktx) and intended for [MVDSV](https://github.com/QW-Group/mvdsv). The ported ServerModules modules are by **Johannes Plass, 1996–1997**, licensed under GPL version 2 or later. KTX, QWProgs and id Software-derived code retain their notices in the respective files.

See [LICENSE.md](LICENSE.md) and [LICENSES/NOTICE.md](LICENSES/NOTICE.md). The code license must not be confused with the license of external game data. This project does not claim endorsement by, or integration into, the official projects.

## Contributions and pull request approval

Pull requests from other contributors require review and approval by **Fernando (Droni) Salvatori, [@fernandosalvatori](https://github.com/fernandosalvatori)** before they can be merged into this repository. The required final approval belongs exclusively to the maintainer; other reviewers' approvals do not replace it.

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidance on submitting reviewable changes, tests and limitations, including how maintainer-authored PRs are handled.
