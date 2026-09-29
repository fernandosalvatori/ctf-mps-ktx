**Language:** English | [Português (Brasil)](NOTICE.pt-BR.md)

# Origins, authorship, and licenses

This document identifies the main sources of the **CTF MPS KTX** code. It supplements the existing file headers; it does not replace or remove their terms.

## Creation and maintenance of the port

**Fernando (Droni) Salvatori**, [@fernandosalvatori](https://github.com/fernandosalvatori), is the creator and maintainer of the **CTF MPS KTX port**, first published on **28 September 2026**.

This attribution refers to the adaptation project and its maintenance. It does not transfer or replace the authorship of KTX, MVDSV, ServerModules, QWProgs, id Software, or earlier contributions. The existing notices remain in their respective files.

The review and final approval required to merge pull requests into this repository are reserved exclusively for [@fernandosalvatori](https://github.com/fernandosalvatori), as described in [CONTRIBUTING.md](../CONTRIBUTING.md). This maintenance policy does not change the components' licenses.

## KTX and earlier code

The `source/ktx` tree is derived from **KTX 1.47**, the [QW-Group/ktx](https://github.com/QW-Group/ktx) project. The text of the GNU General Public License version 2 is in [../LICENSE.md](../LICENSE.md), copied from the KTX distribution.

The KTX files preserve notices from their authors and earlier contributions. These include QWProgs-DM headers with copyright attributed to `[sd] angel` and references to QuakeWorld/Quake code from **id Software, Inc.**, along with other attributions in individual files. These notices have not been replaced with a single attribution to this port.

Also consult the documentation and individual notices in the preserved upstream tree. Some auxiliary components may have their own terms or attributions; this document's summary does not redefine their licenses.

## ServerModules

The special rules were ported from **CTFNormal / ServerModules** QuakeC modules by **Johannes Plass**, copyright **1996, 1997**. Their headers permit redistribution and modification under the **GNU GPL version 2 or, at the recipient's option, any later version** (`GPL-2.0-or-later`).

| QuakeC source | C port |
|---|---|
| `_drone.qc`, `_drone.qh` — Drone 1.0 | `source/ktx/src/ctfnormal_drone.c` |
| `_shrap.qc`, `_shrap.qh` — Shrapnel 1.0 | `source/ktx/src/ctfnormal_shrapnel.c` |
| `_weldgun.qc` — WeldGun 1.0 | `source/ktx/src/ctfnormal_weld.c` |
| `_burn.qc` — Burn 1.0 | `source/ktx/src/ctfnormal_burn.c` |
| `_hook.qc`, `_hook.qh` — Hook 1.2 | `source/ktx/src/ctfnormal_hook.c` |
| `_lightng.qc`, `_lightng.qh` — Lightning 1.1 | Integration in `ctfnormal.c` and `weapons.c` |
| Integrations from `weapons.qc`, `combat.qc`, and `player.qc` | Selection, damage, touch handling, pain, and death in the corresponding KTX files |

The new module files preserve the authorship and GPL notices. The conversion to native C, KTX interfaces, reference cleanup, tests, and documentation are changes made by this port and must not be attributed as behavior originally written by the upstream authors.

## Engine and tools

**MVDSV 1.11** is an external dependency, obtained separately from [QW-Group/mvdsv](https://github.com/QW-Group/mvdsv). Its executable is not distributed in this repository. The MVDSV distribution's license and notices continue to apply to the engine.

Python and Zig are external build and test tools; they are not part of this project's code distribution and retain their own licenses.

## External data not distributed

This repository does not include:

- Quake PAK files;
- BSP maps or entity files from local installations;
- game models and sounds, or third-party resource packages;
- engine executables;
- private configurations, credentials, or copies of personal installations.

The preparation script uses local files supplied by the operator. Providing an importer does not grant additional permission to use or redistribute those files. The GPL covering the mod's code does not automatically make the game's commercial data GPL content.

## Relationship to the original projects

**CTF MPS KTX** identifies this independent adaptation. It does not imply endorsement by KTX, MVDSV, id Software, or the ServerModules authors. Project names are used to identify the code's origins and intended compatibility.
