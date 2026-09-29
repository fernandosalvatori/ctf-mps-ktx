#ifndef CTFNORMAL_H
#define CTFNORMAL_H
#define CFN_WEAPON_ROCKET 1
#define CFN_WEAPON_DRONE 2
#define CFN_WEAPON_SHRAPNEL 4
#define CFN_WEAPON_WELD 8
#define CFN_WEAPON_BURN 16
#define CFN_WEAPON_NAILGUN 32
#define CFN_WEAPON_HOOK 64
qbool CFN_Enabled(void);
void CFN_Precache(void);
void CFN_ClientInit(gedict_t *p);
void CFN_ResetPlayer(gedict_t *p);
void CFN_SelectWeapon(gedict_t *p, int previous_weapon, int new_weapon);
void CFN_Damage(gedict_t *target, gedict_t *inflictor, gedict_t *attacker, float damage, int kind);
void CFN_DronePrecache(void);
void CFN_DroneFire(void);
void CFN_DroneCleanup(gedict_t *p);
void CFN_ShrapnelPrecache(void);
void CFN_ShrapnelFire(void);
void CFN_WeldPrecache(void);
void CFN_WeldFire(vec3_t org, vec3_t dir);
void CFN_BurnPrecache(void);
void CFN_BurnSetOnFire(gedict_t *victim, gedict_t *attacker);
void CFN_BurnCleanup(gedict_t *p);
void CFN_LightningNoise(gedict_t *p, vec3_t target);
void CFN_UpdateWeapon(gedict_t *p);
qbool CFN_Impulse(void);
void CFN_BurnPainSound(void);
void CFN_HookPrecache(void);
void CFN_HookFire(void);
void CFN_HookDestroy(gedict_t *p);
void CFN_HookFrame(void);
#define CFN_HOOK_FLY 16
#endif
