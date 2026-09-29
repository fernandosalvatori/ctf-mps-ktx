/* CTFNormal ServerModules integration for KTX.
 * CTF MPS KTX - mod desenvolvido e mantido por Fernando Salvatori.
 * Copyright (C) 2026 Fernando Salvatori (https://github.com/fernandosalvatori).
 * Development credit refers to this KTX port and integration.
 * Weapon rules derived from ServerModules 1.x, Johannes Plass, 1996-1997.
 * GPL-2.0-or-later. Original sources and provenance accompany this build.
 */
#include "g_local.h"

qbool CFN_Enabled(void)
{
    return cvar("k_ctfnormal") && cvar("k_mode") == 4;
}

void CFN_Precache(void)
{
    if (!CFN_Enabled()) return;
    CFN_DronePrecache();
    CFN_ShrapnelPrecache();
    CFN_WeldPrecache();
    CFN_BurnPrecache();
    CFN_HookPrecache();
}

void CFN_ClientInit(gedict_t *p)
{
    if (!CFN_Enabled()) return;
    stuffcmd_flags(p, STUFFCMD_IGNOREINDEMO,
        "alias help-drone \"impulse 215\"\n"
        "alias help-shrapnel \"impulse 216\"\n"
        "alias help-weldgun \"impulse 217\"\n"
        "alias help-hook \"impulse 219\"\n"
        "alias +hook \"impulse 98\"\n"
        "alias -hook \"impulse 97\"\n");
    G_sprint(p, PRINT_HIGH, "CTFNormal: 4 WeldGun/pregos, 6 granada/Drone, 7 foguete/Shrapnel. help-drone, help-shrapnel, help-weldgun, help-hook\n");
}

void CFN_ResetPlayer(gedict_t *p)
{
    int generation = p->cfn.generation + 1;
    CFN_DroneCleanup(p);
    CFN_BurnCleanup(p);
    CFN_HookDestroy(p);
    memset(&p->cfn, 0, sizeof(p->cfn));
    p->cfn.generation = generation;
}

static void CFN_AnnounceMode(gedict_t *p)
{
    const char *name = p->cfn.weapon_mode == CFN_WEAPON_WELD ? "WeldGun"
        : p->cfn.weapon_mode == CFN_WEAPON_DRONE ? "Drone"
        : p->cfn.weapon_mode == CFN_WEAPON_SHRAPNEL ? "Shrapnel"
        : "arma convencional";
    G_sprint(p, PRINT_HIGH, "Modo: %s\n", name);
}

void CFN_SelectWeapon(gedict_t *p, int previous_weapon, int new_weapon)
{
    int mode = p->cfn.weapon_mode;
    if (!CFN_Enabled()) return;
    p->cfn.weapon_mode = 0;
    if (new_weapon == IT_NAILGUN)
        p->cfn.weapon_mode = (previous_weapon != IT_NAILGUN || mode != CFN_WEAPON_WELD) ? CFN_WEAPON_WELD : 0;
    else if (new_weapon == IT_GRENADE_LAUNCHER && previous_weapon == new_weapon)
        p->cfn.weapon_mode = mode == CFN_WEAPON_DRONE ? 0 : CFN_WEAPON_DRONE;
    else if (new_weapon == IT_ROCKET_LAUNCHER && previous_weapon == new_weapon)
        p->cfn.weapon_mode = mode == CFN_WEAPON_SHRAPNEL ? 0 : CFN_WEAPON_SHRAPNEL;
    p->cfn.last_weapon = new_weapon;
    if (mode != p->cfn.weapon_mode) CFN_AnnounceMode(p);
}

void CFN_UpdateWeapon(gedict_t *p)
{
    if (!CFN_Enabled()) return;
    if (p->cfn.last_weapon != (int)p->s.v.weapon)
        CFN_SelectWeapon(p, p->cfn.last_weapon, p->s.v.weapon);
}

void CFN_Damage(gedict_t *target, gedict_t *inflictor, gedict_t *attacker, float damage, int kind)
{
    int saved = inflictor->cfn.weapon_mode;
    target->deathtype = kind == CFN_WEAPON_DRONE ? dtGL
        : kind == CFN_WEAPON_SHRAPNEL ? dtRL
        : kind == CFN_WEAPON_WELD ? dtNG
        : kind == CFN_WEAPON_HOOK ? dtHOOK : dtNG;
    inflictor->cfn.weapon_mode = kind;
    T_Damage(target, inflictor, attacker, damage);
    inflictor->cfn.weapon_mode = saved;
}

void CFN_LightningNoise(gedict_t *p, vec3_t target)
{
    int choice = 0;
    if (p->cfn.lightning_sound_lasttime < 0 || p->cfn.lightning_sound_nexttime < g_globalvars.time)
        choice = 1;
    if (g_globalvars.time >= p->cfn.lightning_sound_lasttime + 0.1
        && VectorDistance(target, p->cfn.lightning_target) > 10 && g_random() > 0.3)
        choice = 2;
    if (!choice) return;
    sound(p, CHAN_WEAPON, choice == 2 ? "weapons/lstart.wav" : "weapons/lhit.wav", 1, ATTN_NORM);
    p->cfn.lightning_sound_lasttime = g_globalvars.time;
    p->cfn.lightning_sound_nexttime = g_globalvars.time + 0.6;
    VectorCopy(target, p->cfn.lightning_target);
}

qbool CFN_Impulse(void)
{
    int impulse = self->s.v.impulse;
    if (!CFN_Enabled()) return false;
    if (impulse == 97) CFN_HookDestroy(self);
    else if (impulse == 98) CFN_HookFire();
    else if (impulse == 22) {
        if (self->cfn.hook_status) CFN_HookDestroy(self);
        else CFN_HookFire();
    }
    else if (impulse == 215)
        G_sprint(self, PRINT_HIGH, "Drone: selecione o lanca-granadas (6) novamente para alternar. Ate quatro drones perseguem inimigos; gastam foguetes.\n");
    else if (impulse == 216)
        G_sprint(self, PRINT_HIGH, "Shrapnel: selecione o lanca-foguetes (7) novamente para alternar. O projetil libera fragmentos incendiarios.\n");
    else if (impulse == 217)
        G_sprint(self, PRINT_HIGH, "WeldGun: tecla 4 seleciona metal incandescente; repita para pregos. Usa pregos; pode incendiar inimigos.\n");
    else if (impulse == 219)
        G_sprint(self, PRINT_HIGH, "Gancho CTFNormal: bind mouse3 +hook. Segure para lancar/puxar/balancar; solte para liberar. Impulse 22 tambem alterna.\n");
    else return false;
    self->s.v.impulse = 0;
    return true;
}
