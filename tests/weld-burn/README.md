English | [Português (Brasil)](README.pt-BR.md)

# WeldGun and Burn: CTFNormal port for KTX

Origin: `_weldgun.qc` and `_burn.qc` from CTFNormal / ServerModules, version 1.0 modules by Johannes Plass, copyright 1996–1997, GPL version 2 or later. The ported C files preserve the authorship and license; see [NOTICE](../../LICENSES/NOTICE.md).

## Preserved mechanics

### WeldGun

- `weld_blob` projectile, `MOVETYPE_FLYMISSILE`, `SOLID_BBOX`, zero size, speed 1400, and a maximum lifetime of 6 seconds.
- Origin = shot origin − `(0,0,6)` + direction × 8; the model's X angle is increased by 90 degrees.
- Model: `progs/flame2.mdl`; lighting occurs at most once every 0.2 seconds per shooter.
- Original sounds: `weapons/spike2.wav` (volume 0.6), `hknight/idle.wav`, and `wizard/hit.wav` on impact.
- Sky impacts remove the projectile without an explosion or damage. Other impacts produce blood with intensity 9 when the entity hit can take damage.
- Damage radius: 60. Targets without `DAMAGE_AIM` take 10 damage. Other targets receive `db = 11 + (0.5 − r) × 6`, ranging from 8 to 14. Distance is measured from the impact to the target's origin + `(0,0,16)`: full damage below `3 × db`, then a linear falloff to zero at 60.
- Ignition occurs only if the SAME random sample used for damage is strictly greater than 0.85 and the calculated damage is strictly greater than 5. The constant is `0.85f` to preserve QuakeC's float comparison.
- No visibility check was added to area damage: the original QC has none either.
- On impact, the projectile moves back 4 units along its movement vector, stops, and displays frames 0, 3, and 4 of `progs/s_explod.spr` for 0.1 seconds each before being removed.
- Selection, ammunition cost, firing cadence, and player animation belong to the integration in `weapons.c`, rather than `CFN_WeldFire`, preserving the original separation of responsibilities.

### Burn

- Ignition is blocked when the target's water level is > 1, invulnerability is active, or the target is a drone, explosive barrel, dead entity, or teammate other than the attacker.
- Setting yourself on fire remains possible. The port uses native KTX teams in place of the QC `ctf_team` field.
- Up to three independent layers, using bits 1/2/4. Each new layer lasts 15 seconds and deals 3 points per damage tick, for a maximum total of 9 before normal combat modifiers.
- The first think occurs after 0.1 seconds; damage is applied slightly more than 1 second apart because of the strict `time > burn_damage_time` comparison; visual updates occur every 0.02 seconds.
- On expiration, a layer still contributes to the last damage tick before its bit is removed, as in the QC.
- The original nested comparisons are preserved when refreshing three occupied layers. This includes the case where `lifetime1 <= lifetime2` but `lifetime4 < lifetime1`, which refreshes no layer. This historical behavior was not silently corrected.
- Fire spread: centered on the victim's origin + `(0,0,18)`, radius 50, damage `6 + r × 4`, with the strict probability test `r > 0.5`. The same random sample is shared by all nearby entities in that tick; fire spread is credited to the burning player. Direct damage remains credited to the attacker who started the first layer.
- Water above the waist extinguishes the fire on the next damage tick, playing `player/slimbrn2.wav` and producing eight bubbles at intervals of 0.1–0.3 seconds. Bubbles initially rise at 15, use `s_bubble.spr`, and then use KTX's native `bubble_bob` routine.
- Two `flame2.mdl` flames, frame 1, positioned 18 units above the player and 7 units behind their viewing direction, with opposing movements of ±2 forward and ±4 sideways. The height is reduced by 12 for a dead player. The main flame emits light.
- Death from another weapon extinguishes the flames. Death from Burn keeps the effect until `DEAD_DEAD`, then removes the secondary flame and animates frames 0–5 of `s_explod.spr` at 0.1 seconds per frame.
- Pain sounds alternate between `player/lburn1.wav` and `player/lburn2.wav`, with a minimum interval of 0.8 seconds; ignition uses `boss1/throw.wav`.

## KTX API adaptations

- Module fields are stored in `gedict_t.cfn`; native references use `EDICT_TO_PROG` / `PROG_TO_EDICT` for `owner` and `enemy`.
- QC's `findradius` returned a chain. The KTX builtin takes the starting entity for the next search; the port uses its entity iteration without changing the radius, center, or filters.
- QC states of the form `[frame, next_state]` were converted to callbacks with `nextthink = time + 0.1`.
- Models and sounds are explicitly precached, including those that the old module inherited from Quake's general precaching.
- Both flames have references that can be validated and are cleaned up on respawn/disconnect. References are cleared when their lifetime ends; the secondary flame receives classname `burn_flame2` and the steam generator receives `burn_steam`, allowing cleanup without removing another entity.
- The Weld projectile and main flame store the attacker's `connect_time`: if the client disconnects or another player takes over the slot, pending damage is attributed to the world. Normal respawn preserves attribution because it does not change that identifier. The steam generator also stops if the victim's slot is reused. This protection prevents the incorrect attribution that persistent pointers to client slots can cause.
- CTFNormal's separate Protect module is not active under the reference server's `teamplay 40956` setting. The port therefore checks native invulnerability without introducing additional spawn protection.

## Integration contract with the rest of the port

1. Call `CFN_WeldPrecache` and `CFN_BurnPrecache` when precaching the mode.
2. Connect Weld to the nailgun's original selection, ammunition, and firing cadence.
3. `CFN_Damage` must classify the hit as `CFN_WEAPON_WELD` / `CFN_WEAPON_BURN`; `T_Damage` must record `cfn.killweapon` before triggering player death.
4. In `PainSound`, after the water/lava cases and before ordinary pain sounds: if Burn is active on the player, call `CFN_BurnPainSound` and return.
5. On death from Burn: `PlayerDie` selects `player_dieb1`; `PlayerDead` calls `GibPlayer`; `VelocityForDamage` starts from the current velocity and adds random offsets of ±80 in X/Y and 50–100 in Z; `ThrowHead` preserves velocity/height instead of applying the normal −24 offset.
6. Call `CFN_BurnCleanup` on respawn/disconnect, before clearing the fields. Do not clean up at the start of `PlayerDie`, as that would remove the original death effect.

## Validation performed

From the repository root, `python scripts/test.py --zig zig` compiles the two actual C files with this harness and the KTX headers, replacing the engine/KTX boundary with deterministic functions. It requires Python 3.9 or later and Zig 0.13.0 on Windows x64. New results are written to `build/tests/results.json`, alongside the logs. See [BUILD](../../docs/BUILD.md).

The Weld/Burn suite passed **3324 checks** in the CFN1 state. The same command also runs **252 Shrapnel checks** and **98 Drone/Hook checks**, totaling **3674 unit checks**. This count excludes the historical in-engine fixture checks and map sweep.

Coverage includes 280 random-value/impact-distance combinations to validate the formula and ignition probability; flight, lighting, lifetime, and animation frames; ignition blocking; layers and refresh behavior; expiration; attribution of fire spread; water and eight bubbles; ordinary/Burn death; idempotent cleanup; sounds and pain throttling. It also covers attacker disconnection, replacement by another player in the same slot, and legitimate respawn with attribution preserved.

The harness does not run the engine or simulate prediction, networking, or actual map collisions. Client connection, physics, and appearance must be tested in the integrated server. Final damage, including armor, Quad, and team rules, remains the responsibility of `T_Damage`. The separate in-engine fixture checked part of this integration; its scope and limitations are documented in [CHANGES](../../docs/CHANGES.md).
