/* Only linked with -DCFN_TEST. Never present in the installed module. */
#include "g_local.h"
#include <stddef.h>
extern qbool W_ChangeWeapon(int wp);
extern void W_FireSpikes(float ox);
extern void W_FireGrenade(void);
extern void W_FireRocket(void);
extern void W_FireLightning(void);
extern void PlayerDead(void);
static int checks, failures;
static void check(qbool ok, const char *name) {
    ++checks;
    if (!ok) ++failures;
    G_dprint("CFNTEST %s %s\n", ok ? "PASS" : "FAIL", name);
}
static gedict_t *entity(const char *classname) {
    return ez_find(world, (char *)classname);
}
static void ready(gedict_t *p, int weapon) {
    self = p;
    p->attack_finished = 0;
    p->s.v.weapon = weapon;
    p->s.v.health = 1000;
    p->s.v.armorvalue = p->s.v.armortype = 0;
    p->s.v.ammo_rockets = p->s.v.ammo_nails = p->s.v.ammo_cells = 100;
    p->s.v.items = IT_AXE|IT_SHOTGUN|IT_NAILGUN|IT_SUPER_NAILGUN|IT_GRENADE_LAUNCHER|IT_ROCKET_LAUNCHER|IT_LIGHTNING;
    p->invincible_finished = 0;
    p->spawn_time = 0;
    p->s.v.waterlevel = 0;
    p->s.v.deadflag = DEAD_NO;
    p->s.v.takedamage = DAMAGE_AIM;
    p->s.v.solid = SOLID_SLIDEBOX;
    p->s.v.movetype = MOVETYPE_WALK;
    p->s.v.flags = FL_CLIENT;
    p->cfn.last_weapon = weapon;
    p->cfn.weapon_mode = 0;
    W_SetCurrentAmmo();
}
int CFN_TestConsole(void) {
    char cmd[64];
    gedict_t *p,*target,*e,*save = self;
    int a,b, i;
    float health;
    trap_CmdArgv(1,cmd,sizeof(cmd));
    if (!strcmp(cmd,"cfn_mapcheck")) {
        qbool valid=isCTF() && entity("item_flag_team1") && entity("item_flag_team2");
        if (!strcmp(mapname,"e4m4")) {
            e=find(world,offsetof(gedict_t,targetname),"t204");
            valid=valid && e && fabs(e->s.v.origin[0]-1065)<0.1 && fabs(e->s.v.origin[1]-758)<0.1 && fabs(e->s.v.origin[2]-300)<0.1 && fabs(e->mangle[0]-30)<0.1 && fabs(e->mangle[1]-102)<0.1;
        }
        if (!strcmp(mapname,"e4m2")) valid=valid && !entity("item_key1") && !entity("item_key2");
        G_dprint("CFNMAP %s %s\n", valid ? "PASS" : "FAIL", mapname);
        return 1;
    }
    if (strcmp(cmd,"cfn_test")) return 0;
    checks=failures=0;
    check(isCTF() && CFN_Enabled(),"native CTF active");
    check(entity("item_flag_team1") && entity("item_flag_team2"),"both CTF flags exist");
    a=trap_AddBot("CFN-Test-A",4,4,"base");
    b=trap_AddBot("CFN-Test-B",13,13,"base");
    if (!a || !b) { G_dprint("CFNTEST FAIL fixture slots\n"); return 1; }
    p=&g_edicts[a];target=&g_edicts[b];
    trap_SetBotUserInfo(a,"team","red",0);
    trap_SetBotUserInfo(b,"team","blue",0);
    match_in_progress=2;
    ready(target,IT_SHOTGUN);
    ready(p,IT_SHOTGUN);
    W_ChangeWeapon(4);
    check(p->cfn.weapon_mode==CFN_WEAPON_WELD,"first nailgun selects WeldGun");
    W_FireSpikes(4);
    e=entity("weld_blob");
    check(e && p->s.v.ammo_nails==99,"WeldGun spawn and one nail cost");
    check(e && fabs(VectorLength(e->s.v.velocity)-1400)<1,"WeldGun speed 1400");
    p->attack_finished=0;W_ChangeWeapon(4);
    check(p->cfn.weapon_mode==0,"repeat nailgun selects conventional nails");
    p->attack_finished=0;W_ChangeWeapon(6);W_ChangeWeapon(6);
    check(p->cfn.weapon_mode==CFN_WEAPON_DRONE,"repeat grenade selects Drone");
    W_FireGrenade();
    check(entity("drone") && p->s.v.ammo_rockets==99,"Drone integration and ammunition");
    for(i=0;i<5;i++) CFN_DroneFire();
    check((int)p->cfn.drone_count==15,"Drone four-slot queue stays bounded");
    CFN_DroneCleanup(p);
    check(p->cfn.drone_count==0,"Drone cleanup clears queue");
    p->attack_finished=0;W_ChangeWeapon(7);W_ChangeWeapon(7);
    check(p->cfn.weapon_mode==CFN_WEAPON_SHRAPNEL,"repeat rocket selects Shrapnel");
    health=p->s.v.ammo_rockets;W_FireRocket();
    e=entity("shrapnel_projectile");
    check(e && p->s.v.ammo_rockets==health-1,"Shrapnel integration and ammunition");
    check(e && fabs(VectorLength(e->s.v.velocity)-850)<1,"Shrapnel speed 850");
    ready(target,IT_SHOTGUN);ready(p,IT_SHOTGUN);
    CFN_BurnSetOnFire(target,p);
    check(target->cfn.burn_burning!=0,"Burn attaches to enemy");
    e=target->cfn.burn_flame;
    if (!e || !e->think) e=entity("burn_flame");
    if (e && e->think) {
        e->cfn.burn_damage_time=0;self=e;health=target->s.v.health;
        ((void(*)(void))e->think)();
        check(target->s.v.health<health,"Burn inflicts actual KTX damage");
        target->s.v.waterlevel=3;e->cfn.burn_damage_time=0;self=e;
        ((void(*)(void))e->think)();
        check(target->cfn.burn_burning==0,"water extinguishes burning");
    } else check(false,"Burn controller exists");
    CFN_BurnCleanup(target);
    ready(target,IT_SHOTGUN); ready(p,IT_SHOTGUN);
    target->s.v.health=2;
    CFN_Damage(target,p,p,3,CFN_WEAPON_BURN);
    check(target->cfn.killweapon==CFN_WEAPON_BURN && target->s.v.deadflag!=DEAD_NO,"Burn lethal damage enters player death");
    self=target; PlayerDead();
    check(target->cfn.burn_gibbed==1,"Burn death finishes with gib");
    CFN_ResetPlayer(target);ready(target,IT_SHOTGUN);
    check(target->cfn.killweapon==0 && target->cfn.burn_gibbed==0,"respawn resets Burn death state");
    target->s.v.health=2;
    target->deathtype=dtSG; T_Damage(target,p,p,3);
    check(target->cfn.killweapon==0,"ordinary death does not inherit Burn");
    CFN_ResetPlayer(target);ready(target,IT_SHOTGUN);ready(p,IT_SHOTGUN);
    trap_SetBotUserInfo(b,"team","red",0);
    health=target->s.v.health;CFN_Damage(target,p,p,20,CFN_WEAPON_WELD);
    check(target->s.v.health==health,"teamplay protects allies from special damage");
    CFN_BurnSetOnFire(target,p);
    check(!target->cfn.burn_burning,"ally protection prevents ignition");
    health=p->s.v.health;CFN_Damage(p,p,p,20,CFN_WEAPON_SHRAPNEL);
    check(p->s.v.health<health,"teamplay preserves self damage");
    ready(p,IT_LIGHTNING);p->s.v.waterlevel=3;health=p->s.v.health;
    W_FireLightning();
    check(p->s.v.ammo_cells==0,"submerged lightning consumes cells");
    check(health-p->s.v.health>0 && health-p->s.v.health<=400,"submerged lightning damage capped at 400");
    ready(p,IT_SHOTGUN);self=p;
    p->s.v.impulse=98;CFN_Impulse();
    check(p->cfn.hook_status!=0 && p->cfn.hook_next,"original hook launches via 98");
    p->s.v.impulse=97;CFN_Impulse();
    check(!(p->cfn.hook_status&1),"original hook releases via 97");
    CFN_ResetPlayer(p);CFN_ResetPlayer(target);
    trap_RemoveBot(a);trap_RemoveBot(b);
    self=save;
    G_dprint("CFNTEST RESULT checks=%d failures=%d\n",checks,failures);
    return 1;
}


