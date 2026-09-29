# CTF MPS KTX — technical port change log

English | [Português (Brasil)](CHANGES.pt-BR.md)

Documented state: **CFN1**, module `1.47-ctfnormal.1`, derived from **KTX 1.47**, with integration tests run on **MVDSV 1.11 Windows x64**.

**Port creator and maintainer:** Fernando (Droni) Salvatori — [@fernandosalvatori](https://github.com/fernandosalvatori). **First published: 28 September 2026.** Attribution for this port does not replace the authors of the code and modules on which it is based.

The purpose is to port the weapons and effects from **CTFNormal / ServerModules** to native KTX C: Drone, Shrapnel, WeldGun, Burn, the Lightning modification, and Hook 1.2. CTF match rules, networking, physics, and movement prediction remain the responsibility of KTX/MVDSV. This record does not claim complete equivalence between a NetQuake match and a QuakeWorld match.

## Origins and attribution

- KTX: [QW-Group/ktx](https://github.com/QW-Group/ktx), based on version 1.47, with its authorship and license headers preserved.
- Test engine: [QW-Group/mvdsv](https://github.com/QW-Group/mvdsv), version 1.11, obtained and run separately.
- Ported modules: ServerModules by **Johannes Plass**, copyright 1996–1997, **GPL version 2 or later**.
- QuakeC references: `_drone.qc`, `_shrap.qc`, `_weldgun.qc`, `_burn.qc`, `_hook.qc`, `_lightng.qc`, their respective headers, and integration points in `weapons.qc`, `combat.qc`, and `player.qc`.

Notices for KTX, QWProgs, and code derived from id Software remain in their respective files. [LICENSES/NOTICE.md](../LICENSES/NOTICE.md) explains the relationship between the code, external tools, and game data. The port does not replace component authorship with a single attribution to the new project.

## Organization and changed files

| File | Port responsibility |
|---|---|
| `source/ktx/include/ctfnormal.h` | Weapon constants and the extension's public interfaces |
| `source/ktx/include/progs.h` | `cfn_state_t`: private state, targets, timers, logical owners, and the hook chain |
| `source/ktx/include/g_local.h` | Interface inclusion and custom version identification |
| `source/ktx/src/ctfnormal.c` | Activation, precaching, aliases, help, selection, messages, cleanup, and the damage wrapper |
| `source/ktx/src/ctfnormal_drone.c` | Drone queue, navigation, contacts, damage, and explosions |
| `source/ktx/src/ctfnormal_shrapnel.c` | Missile/flame, fragments, reflection, damage, and animations |
| `source/ktx/src/ctfnormal_weld.c` | Incandescent projectiles, lighting, impacts, and ignition |
| `source/ktx/src/ctfnormal_burn.c` | Fire stacks, spread, flames, steam, pain, and removal |
| `source/ktx/src/ctfnormal_hook.c` | Hook, links, attachment, pulling/swinging, and removal |
| `source/ktx/src/weapons.c` | Integration of selection, firing, ammunition, firing intervals, and Lightning |
| `source/ktx/src/combat.c` | Identification of the lethal hit and special handling of damage to drones |
| `source/ktx/src/player.c` | Fire pain and death animation, gib velocities, and hook removal |
| `source/ktx/src/client.c` | Client initialization/cleanup, alerts, and death messages |
| `source/ktx/src/commands.c` | CTF preset selection for the extension's continuous-play mode |
| `source/ktx/src/world.c` | Cvar registration, precaching, and correct CTF mode identification |
| `source/ktx/CMakeLists.txt` | Inclusion of the new C units |
| `tests/` | Deterministic tests and a dedicated integration fixture |
| `scripts/` | Building, running tests, and preparing external data locally |
| `server-example/ktx/` | Example configurations without game data or credentials |

Engine test code is conditional on `CFN_TEST`. Its entry point is for validation and is not part of the production build's administration interface.

## Activation and lifecycle

`CFN_Enabled()` requires `k_ctfnormal` to be enabled and `k_mode == 4`. Precaching explicitly registers the models and sounds used by the modules. Optional Hook resources that were disabled in the reference source remain disabled, avoiding dependencies on additional files not expected by that configuration.

`ClientConnect` provides aliases and help. `PutClientInServer` and `ClientDisconnect` call cleanup before reusing private state. Cleanup removes drones and flames, requests hook removal, clears `cfn`, and advances the player's generation counter. The hook checks its generation and its link to the player so it cannot control a later life or connection.

`PlayerPreThink` participates in hook removal when the player dies, teleports, or leaves the mode. It also reports changes in the Drone threat state through text messages.

### Weapon selection

- Selecting the nailgun from another weapon enters WeldGun mode; selecting it again toggles between Weld and ordinary nails.
- The GL and RL initially retain their conventional behavior; selecting them again toggles Drone or Shrapnel.
- Automatic weapon changes also update this state, preventing an incompatible alternative mode from carrying over to another weapon.
- Drone and Shrapnel deduct ammunition inside their firing functions. The calling code does not deduct it again.
- Weld uses the nailgun firing path's one-nail deduction and preserves the original lateral correction, `ox -= 1`.
- The integration retains the specific shotgun, super shotgun, GL, and RL firing intervals in CTFNormal mode. Firing intervals are separate from projectile and effect `think` intervals.

The system uses `Modo: ...` messages ("Mode: ...") instead of the old inventory-key indicator. Key bits are used to display flags in QuakeWorld CTF; reusing them for weapons would corrupt that display.

## Drone 1.0

| Element | Ported behavior/value |
|---|---|
| Cost | 1 rocket |
| Queue | Up to four drones; a new launch schedules the oldest one to explode |
| Health | 20 points |
| Initial speed | 400 units/s |
| First `think` | 0.6 s after launch |
| Subsequent updates | 0.2 s |
| Lifetime | Original logic of approximately 6 s, subject to the expiry margin and `think` timing resolution |
| Search | Flight direction, visibility of target positions, distance, and compatible submersion levels |
| Movement | Updates to estimated target position/velocity, speed adjustment, and deviations when heading in the opposite direction |
| Collision | Ricochets, damage from contacts, and recovery when stuck or on the ground |
| Radial damage | Radius 70, base `40 + r × 5`, reduced by distance |
| Ignition | `r > 0.95` and damage > 20 |
| Model | `progs/lavaball.mdl` |
| Explosion | `progs/s_explod.spr`, frames 0–5 |

The Drone keeps a logical owner separate from the physical owner used for collisions. Its queue and references are removed during player cleanup. The port also handles drones that were pursuing the removed player, preventing them from unintentionally targeting a client that reuses that slot.

Two reference-handling issues were fixed: removal on sky contact now unlinks the Drone from its queue, and the no-target search check compares against the sentinel actually used (−10). The latter fixes a historical comparison against −1. These are deliberate differences and should not be described as a literal reproduction of a QC bug.

## Shrapnel 1.0

| Element | Ported behavior/value |
|---|---|
| Cost | 1 rocket |
| Main missile | Speed 850, maximum lifetime 6 s |
| Missile origin | Player origin + forward × 36 + Z14 |
| Accompanying flame | Forward × 18 + Z14, same speed and lifetime |
| Sky contact | Removal without an explosion |
| Primary explosion | On BSP contact, base damage `30 + r × 10`, search radius damage + 40 |
| Primary falloff | Damage − half the distance to the bounding-box center; halved against the shooter and a shambler; requires `CanDamage` |
| Fragments | Normally three; four depending on the primary explosion's random roll |
| Fragment speed | 600 units/s |
| Fragment damage | Radius 70; base `25 + (0.5 − r) × 5` against `DAMAGE_AIM` |
| Fragment falloff | Base damage near the center, then `(70 − distance) / 2` down to zero |
| Other damageable objects | 10 points |
| Ignition | `r > 0.66` and damage > 6 |
| BSP contact timing | Independent damage and sound intervals, each strictly greater than 0.1 s |
| Fragment explosion | Frames 0, 3, 4, then removal; 0.1 s per frame |

Non-BSP impacts do not receive the primary explosion that exists only for BSP contacts in the original. Fragments retain ricochets and their return to flight. The fragment formula does not add a line-of-sight check absent from the QC and does not automatically halve damage against the shooter.

Damage attribution uses the logical owner, even when the fragment's physical owner is the world. The owner's connection time is saved to prevent credit from transferring when a slot is reused.

## WeldGun 1.0

| Element | Ported behavior/value |
|---|---|
| Projectile | `weld_blob`, `MOVETYPE_FLYMISSILE`, zero-size bounding box |
| Cost | 1 nail, deducted by the nailgun firing path |
| Speed | 1400 units/s |
| Maximum lifetime | 6 s |
| Origin | Shot origin − Z6 + direction × 8 |
| Light | At most one illuminated projectile every 0.2 s per shooter |
| Model | `progs/flame2.mdl`, X angle increased by 90 degrees |
| Radius | 60 |
| Damage against `DAMAGE_AIM` | `db = 11 + (0.5 − r) × 6`, between 8 and 14 |
| Damage distance | From target origin + Z16 to the impact |
| Falloff | Full damage below `3 × db`, decreasing linearly to zero at 60 |
| Other damageable objects | 10 points |
| Ignition | The same `r` sample used for damage: strictly > 0.85, with damage > 5 |
| Explosion | Moves back 4 units; frames 0, 3, 4 in `s_explod.spr`, 0.1 s each |

Sky impacts are removed without damage. The original firing, flight, and impact sounds are retained. The `0.85f` comparison preserves the QuakeC constant's float precision: it must not become a comparison against a double constant that includes the boundary value because of rounding.

No wall check was added to radial damage because the original routine did not have one. Final damage goes through KTX's normal combat modifiers.

## Burn 1.0

### Ignition and stacks

Ignition is blocked when the victim is submerged above the waist, invulnerable, dead, a Drone, or an explosive barrel. In CTF, it cannot ignite another member of the same team; self-ignition remains possible under the original rules.

Stacks use bits 1, 2, and 4, each lasting 15 seconds. Each stack contributes 3 points per damage tick; three stacks total 9 before armor and other modifiers. The first `think` occurs after 0.1 s; damage uses a 1 s timer and the strict comparison `time > burn_damage_time`. Visuals update every 0.02 s.

An expired stack still contributes on the tick that removes its bit, as in the QC. When all three stacks are occupied, the original nested refresh comparisons are preserved, including the case where no stack is refreshed. This quirk is not presented as a new balancing decision.

### Spread and water

Spread uses a center at victim origin + Z18, radius 50, damage `6 + r × 4`, and ignition when `r > 0.5`. The same random roll is used for nearby entities during that tick. Spread damage is credited to the burning character; direct stack damage remains attributed to the initial attacker.

Water with `waterlevel > 1` extinguishes the fire on the next damage tick, producing an extinguishing sound and eight bubbles at intervals of 0.1–0.3 s. Bubbles start with velocity Z15 and then use the native `bubble_bob` routine.

### Flames, pain, and death

Two flames use `flame2.mdl`, frame 1, near the character and behind the viewing direction, with opposite positional jitter. The main flame emits light. Their height is reduced during the death animation.

`PainSound` uses burning pain sounds with a minimum interval of 0.8 s. Death from another weapon removes the flames; death from Burn retains the effect until `DEAD_DEAD` and ends with an explosion animation using frames 0–5.

The player death sequence uses `player_dieb1`, followed by gibs. Gib velocity starts from the current velocity, adding ±80 on X/Y and 50–100 on Z. `ThrowHead` preserves the height and velocity specific to a fire death. The `burn_gibbed` flag prevents duplicate gibs in the KTX flow when they have already been produced before `PlayerDead`.

Flames are only cleaned up on a new life/disconnection or when the effect ends, not at the start of `PlayerDie`. Flag dropping remains in the native CTF flow.

## Lightning 1.1 and Hook 1.2

### Lightning

The special discharge requires `waterlevel > 2`. Its base intensity is `min(400, 20 × cells)`, and it consumes the remaining cells. Distance, armor, and other rules still apply; 400 must not be described as guaranteed final damage to every target.

Sounds are controlled by timers and movement of the impact point. The movement check uses a minimum interval of 0.1 s, a change greater than 10 units, and a random roll > 0.3; regular repetition uses the original 0.6 s window.

### Hook

Impulses 98/97 implement hold/release. Impulse 22 is an additional shortcut for toggling the same system. The hook travels at 1400 units/s, creates eight links, and deals 7 points of contact damage before modifiers. Attachment to a character lasts at most 2 s.

Pulling and swinging are calculated by the original `think` at 0.1 s intervals, combining velocity components parallel and tangential to the attachment point. The effect is not replaced with the native KTX hook. Death, teleportation, and release end the connection.

The reference code had commented out the block that activated `HOOK_FLY`; the port does not re-enable it as new behavior. Optional custom models and sounds that were disabled in the reference remain disabled. The standard `v_spike.mdl`, `s_spike.mdl`, and corresponding sounds are retained.

## API adaptations and reference-safety fixes

1. **QC state to C:** private fields live in `gedict_t.cfn`; native `owner`/`enemy` fields use `EDICT_TO_PROG` and `PROG_TO_EDICT`. QuakeC state functions `[frame, next]` became callbacks with an explicit `nextthink`.
2. **Radius searches:** the linked chain returned by the NetQuake builtin was replaced with incremental iteration through KTX's `trap_findradius`. The specific centers, radii, and filters were retained.
3. **Shared damage path:** `CFN_Damage` classifies the weapon and forwards to `T_Damage`; it does not bypass player armor, Quad, or team protection. The lethal hit records `cfn.killweapon` before `Killed`.
4. **Reused slots:** Weld, Shrapnel, and Burn record the attacker's connection identity. If someone else occupies the same slot, pending damage is attributed to the world. A normal respawn keeps the credit because the connection has not changed.
5. **Effect cleanup:** flame references are cleared when the effect ends. The second flame and the steam emitter have their own identifiers. Steam does not follow a new client that reuses the victim's slot.
6. **Hook:** the link and generation prevent an old hook from modifying the state of a later life. The chain and callbacks are removed along with the hook.
7. **Drone:** queue cleanup on sky contact and the no-target sentinel fix prevent incorrect references from being used.
8. **Death:** recording the weapon only on the lethal hit and guarding against duplicate gibs preserve death state even when additional calls deal no effective damage.

These changes must be distinguished from the historical formulas that were retained. Fidelity does not require keeping invalid references or crediting damage to the wrong occupant of a slot.

## CTF configuration and loading

The example uses continuous CTF with `k_matchless 1`. When `k_ctfnormal` is active, automatic mode selection chooses the CTF preset. Publishing `mode=ctf` also considers the native `isCTF()` state to avoid displaying a previous preset's label on the first load.

Common preset initialization clears `sv_loadentfiles_dir`. The mode rules restore `sv_loadentfiles 1` and `sv_loadentfiles_dir ctf` directly in the appropriate files. Setting these options only before a preset was not enough to ensure entity loading.

The reference configuration provides a score limit of 150, time limit of 40, speed of 350, and acceleration of 20. These are example settings, not code requirements. `teamplay 4` protects teammates' health/armor while retaining self-damage; the old ServerModules bitmask is not reused as a KTX teamplay number.

Bots and runes are disabled, as is the native KTX hook. The build may contain native bot-support structures required by the KTX base; this does not create bots or populate the server. The fixture's synthetic clients are for testing only.

Map data is external. Preparing an installation uses locally supplied files and does not modify the originals. Entity consistency fixes must be checked against the corresponding BSP: a submodel name does not identify the same geometry across different maps.

The importer applies two adjustments to the local entity copies: in `e4m4`, teleport destination `t204` uses origin `1065 758 273` and angles `30 102 0`, so the native Z27 addition during spawn produces `1065 758 300`; in `e4m2`, it removes ordinary keys active in deathmatch without removing flag entities. This reproduces adjustments that the CTFNormal code made after loading. The importer does not distribute these files or turn that data into repository content.

## Recorded validation

The numbers below describe the CFN1 state. All three unit suites were rerun in the public tree: **3674 checks passed** on Windows x64 with Zig 0.13.0. The native build and DLL loading, including the `vmMain` and `dllEntry` exports, also passed. The 27 engine checks and the 20-map sweep are earlier records of the same state, not part of the unit-test command. The suites should be rerun when code, the compiler, or relevant integrations change.

| Suite | Total | Scope |
|---|---:|---|
| WeldGun/Burn | 3324 | Actual C modules; mocked KTX/engine boundary |
| Shrapnel | 252 | Actual C module; deterministic inputs and responses |
| Drone/Hook | 98 | Actual C modules; queue, targets, movement, and states |
| Unit total | **3674** | Does not replace engine physics/rendering tests |
| MVDSV/KTX integration | **27** | Fixture loaded in the engine, zero failures |
| Rotation | **20 maps** | Loading, status, CTF mode, and both flags |

### What the unit tests exercise

- **Weld/Burn:** 280 combinations of random value/impact distance, radial formula, strict thresholds, flight, lighting, lifetime, frames, ignition blockers, stacking/refresh/expiry, spread attribution, water/bubbles, death, idempotent cleanup, sounds, and connection identity.
- **Shrapnel:** launch/cost, model/ownership, sky, BSP and non-BSP, dispersion and fragment count, damage formulas, ignition, the shambler exception, obstacles for the primary explosion, blood, rate limits, ricochets, return to flight, expiry, animations, and owner-slot reuse.
- **Drone/Hook:** ported paths with deterministic engine responses, including the queue limit, target selection/updates, contact states, hook/chain behavior, and removal.

### What the engine fixture exercises

1. Native CTF active and both flags present.
2. Weld selection when entering the nailgun and switching back to ordinary nails.
3. Weld projectile creation, ammunition, and speed.
4. Switching the GL to Drone, its cost, and the four-drone limit.
5. Drone queue cleanup.
6. Switching the RL to Shrapnel, its cost, and speed.
7. Ignition, actual health loss through `T_Damage`, and extinguishing in water.
8. Burn death, gibs, reset on respawn, and a subsequent conventional death.
9. Teammate protection against damage/ignition and preservation of self-damage.
10. Cell consumption and the submerged Lightning discharge cap.
11. Hook launch/release through impulses 98/97.

These groups contain 27 individual checks. The fixture is compiled with `CFN_TEST`; its entry point must not be included in the normal build.

### Map sweep

The following maps were loaded: `e1m1`, `e1m2`, `e1m3`, `e1m4`, `e1m5`, `e1m6`, `e2m1`, `e2m2`, `e2m3`, `e2m5`, `e3m1`, `e4m3`, `e4m4`, `e4m5`, `e4m6`, `dm1`, `dm3`, `dm4`, `dm5`, and `dm6`.

The test confirmed a status response with the correct map, active CTF, and both flags. It did not visually traverse every route or run a complete match on each map. The BSP/ENT files used in that test are not distributed in the repository.

### Validation limits

The following have not yet been demonstrated:

- a complete human match with visual and control checks for every weapon;
- identical collisions, ricochets, aiming, and movement feel compared with the original NetQuake mod;
- flag capture/return under every combination of effects, hook use, and deaths;
- a complete matrix of team changes, inventory/HUD behavior, latency, and multiple clients;
- sustained match load and equivalence across other operating systems/architectures.

The project records what was actually tested. Assertion counts, successful builds, and map loading are not used as substitutes for a human playtest.

## Regressions and future review

Pull requests to this repository require review and final approval from [@fernandosalvatori](https://github.com/fernandosalvatori). Other reviews help assess changes but do not replace the maintainer's exclusive approval to merge them. The expected contribution format is described in [CONTRIBUTING.md](../CONTRIBUTING.md).

Changes to this port should preserve the existing suites and add focused cases when modifying damage, selection, lifecycle, or entity ownership. The highest-risk areas are state that survives respawn/disconnection, damage callbacks that trigger death, entity queues/chains, and rule loading during map changes.

An upstream integration proposal should separate weapon code from configuration preferences, preserve ServerModules authorship/license notices, provide the diff against the KTX base, and include human gameplay results when available. Commercial game data, external resources, and private configurations do not belong in the code patch.

The public tree documented here does not imply that a PR has been accepted or that the original maintainers have approved this adaptation.
