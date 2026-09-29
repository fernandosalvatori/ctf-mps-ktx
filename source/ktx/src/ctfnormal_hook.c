/* ServerModules Hook 1.2, Copyright (C) 1996, 1997 Johannes Plass.
 * Port of ctfnormal/src/_hook.qc and _hook.qh to KTX.
 * GPL version 2 or (at your option) any later version. See COPYING.
 */
#include "g_local.h"
#include "ctfnormal.h"

#define HOOK_ACTIVE 1
#define HOOK_EXISTS 2
#define HOOK_FLY 16
#define HOOK_SWING 32
#define HOOK_SWITCHED_SOUND_PULL 64
#define HOOK_SWITCHED_SOUND_SWING 128
#define HOOK_FRAME_73 256
#define HOOK_FRAME_72 512
#define HOOK_FRAME_71 1024
#define HOOK_FRAME_70 2048
#define HOOK_FRAME_8 4096
#define HOOK_DONT_FLY 8192

static void HookRemove(void);
static void HookThink(void);
static void HookTouch(void);

static gedict_t *HookOwner(gedict_t *h) { return PROG_TO_EDICT(h->s.v.owner); }
static int HookOwnsPlayer(gedict_t *h)
{
	gedict_t *p = HookOwner(h);
	return p != world && p->cfn.hook_next == h && p->cfn.generation == h->cfn.generation;
}

void CFN_HookPrecache(void)
{
	/* These are exactly the original HOOK_USE_CUSTOM_* = 0 resources. */
	trap_precache_sound("player/axhit2.wav");
	trap_precache_sound("shambler/smack.wav");
	trap_precache_sound("misc/power.wav");
	trap_precache_model("progs/v_spike.mdl");
	trap_precache_model("progs/s_spike.mdl");
}

static void HookChangeFrame(float frame, int newflag, int oldflag)
{
	gedict_t *p = HookOwner(self);
	p->s.v.frame = frame;
	self->cfn.hook_status = (self->cfn.hook_status | newflag) & ~oldflag;
	if (frame >= 70) {
		p->weaponmodel = "";
		p->s.v.weaponframe = 0;
		p->s.v.weapon = 0;
	}
}

static void HookAnimateReturn(void)
{
	gedict_t *p = HookOwner(self);
	if (self->cfn.hook_status & HOOK_FRAME_73) HookChangeFrame(72, HOOK_FRAME_72, HOOK_FRAME_73);
	else if (self->cfn.hook_status & HOOK_FRAME_72) HookChangeFrame(71, HOOK_FRAME_71, HOOK_FRAME_72);
	else if (self->cfn.hook_status & HOOK_FRAME_71) HookChangeFrame(70, HOOK_FRAME_70, HOOK_FRAME_71);
	else if (self->cfn.hook_status & HOOK_FRAME_70) {
		HookChangeFrame(8, HOOK_FRAME_8, HOOK_FRAME_70);
		p->weaponmodel = self->weaponmodel;
		p->s.v.weaponframe = self->s.v.weaponframe;
		p->s.v.weapon = self->s.v.weapon;
	} else HookChangeFrame(8, HOOK_FRAME_8, 0);
}

static void HookThink(void)
{
	gedict_t *h = self, *p = HookOwner(h), *enemy = PROG_TO_EDICT(h->s.v.enemy);
	vec3_t x, velocity, parallel, tangential, center;
	float distance, tangent_speed, acceleration, speed;
	if (!HookOwnsPlayer(h) || !(p->cfn.hook_status & HOOK_ACTIVE)
		|| p->teleport_time > g_globalvars.time || p->s.v.deadflag
		|| enemy->s.v.solid == SOLID_NOT) { HookRemove(); return; }
	if (enemy->s.v.solid == SOLID_SLIDEBOX) {
		if (h->cfn.hook_touch_time + 2 <= g_globalvars.time) { HookRemove(); return; }
		VectorAdd(enemy->s.v.origin, enemy->s.v.mins, center);
		VectorMA(center, 0.5f, enemy->s.v.size, center);
		setorigin(h, PASSVEC3(center));
	}
	h->s.v.nextthink = g_globalvars.time + 0.1f;
	VectorCopy(enemy->s.v.velocity, h->s.v.velocity);
	VectorCopy(p->s.v.velocity, velocity);
	VectorSubtract(h->s.v.origin, p->s.v.origin, x); x[2] -= 16;
	distance = VectorNormalize(x);
	if (distance > 0) VectorScale(x, DotProduct(velocity, x), parallel);
	else VectorClear(parallel);
	VectorSubtract(velocity, parallel, tangential);
	tangent_speed = vlen(tangential);
	if (tangent_speed > distance + 100)
		VectorScale(tangential, (tangent_speed - 30) / tangent_speed, tangential);
	if (p->cfn.hook_status & HOOK_SWING) acceleration = distance < 240 ? 2.5f * distance : 600;
	else {
		if (distance < 100) acceleration = 6 * distance;
		else { acceleration = 600; VectorClear(tangential); }
		if (distance < 100 && DotProduct(parallel, x) <= 0) p->cfn.hook_status |= HOOK_SWING;
	}
	VectorMA(parallel, 0.5f * acceleration, x, parallel);
	speed = vlen(parallel);
	if (speed > acceleration) VectorScale(parallel, acceleration / speed, parallel);
	speed = vlen(tangential);
	if (speed > 600) VectorScale(tangential, 600 / speed, tangential);
	VectorAdd(tangential, parallel, p->s.v.velocity);
	/* The original code computing and toggling HOOK_FLY is commented out.
 * Preserve that disabled behavior; retain its animation transitions. */
	if (p->cfn.hook_status & HOOK_FLY) {
		if (h->cfn.hook_status & HOOK_FRAME_8) {
			h->weaponmodel = p->weaponmodel;
			h->s.v.weaponframe = p->s.v.weaponframe;
			h->s.v.weapon = p->s.v.weapon;
			HookChangeFrame(70, HOOK_FRAME_70, HOOK_FRAME_8);
		} else if (h->cfn.hook_status & HOOK_FRAME_70) HookChangeFrame(71, HOOK_FRAME_71, HOOK_FRAME_70);
		else if (h->cfn.hook_status & HOOK_FRAME_71) HookChangeFrame(72, HOOK_FRAME_72, HOOK_FRAME_71);
		else if (h->cfn.hook_status & HOOK_FRAME_72) HookChangeFrame(73, HOOK_FRAME_73, HOOK_FRAME_72);
		else HookChangeFrame(73, HOOK_FRAME_73, 0);
	} else HookAnimateReturn();
	/* Custom chain sounds were disabled in ctfnormal; preserve the state
 * transitions without introducing unavailable optional WAV assets. */
	if (!(h->cfn.hook_status & HOOK_SWITCHED_SOUND_SWING) && (p->cfn.hook_status & HOOK_SWING))
		h->cfn.hook_status |= HOOK_SWITCHED_SOUND_SWING | HOOK_SWITCHED_SOUND_PULL;
	if (!(h->cfn.hook_status & HOOK_SWITCHED_SOUND_PULL) && h->cfn.hook_touch_time + 0.3f < g_globalvars.time)
		h->cfn.hook_status |= HOOK_SWITCHED_SOUND_PULL;
}

static void HookChainThink(void)
{
	gedict_t *h = HookOwner(self), *p = HookOwner(h);
	vec3_t x, origin;
	if (!HookOwnsPlayer(h)) { ent_remove(self); return; }
	VectorSubtract(h->s.v.origin, p->s.v.origin, x); x[2] -= 16;
	VectorMA(h->s.v.origin, -self->cfn.hook_xn, x, origin);
	setorigin(self, PASSVEC3(origin));
	vectoangles(x, self->s.v.angles);
	self->s.v.nextthink = g_globalvars.time + 0.1f;
}

static void HookTouch(void)
{
	gedict_t *h = self, *p = HookOwner(h), *target = other;
	vec3_t center, delta;
	float min_distance;
	if (trap_pointcontents(PASSVEC3(h->s.v.origin)) == CONTENT_SKY
		|| !HookOwnsPlayer(h) || !(p->cfn.hook_status & HOOK_ACTIVE)) { HookRemove(); return; }
	if (target->s.v.takedamage) CFN_Damage(target, h, p, 7, CFN_WEAPON_HOOK);
	if (target->s.v.solid == SOLID_SLIDEBOX) {
		sound(h, CHAN_WEAPON, "shambler/smack.wav", 1, ATTN_NORM);
		SpawnBlood(h->s.v.origin, 10);
		VectorAdd(target->s.v.origin, target->s.v.mins, center);
		VectorMA(center, 0.5f, target->s.v.size, center);
		setorigin(h, PASSVEC3(center));
		h->cfn.hook_status |= HOOK_DONT_FLY;
	} else sound(h, CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM);
	VectorSubtract(h->s.v.origin, p->s.v.origin, delta); delta[2] -= 16;
	min_distance = p->s.v.waterlevel > 2 ? 200 : 520;
	if (vlen(delta) < min_distance) h->cfn.hook_status |= HOOK_DONT_FLY;
	VectorClear(h->s.v.avelocity);
	VectorCopy(target->s.v.velocity, h->s.v.velocity);
	h->s.v.enemy = EDICT_TO_PROG(target);
	h->s.v.nextthink = g_globalvars.time + 0.1f;
	h->think = (func_t)HookThink;
	h->touch = (func_t)SUB_Null;
	h->cfn.hook_touch_time = p->cfn.hook_touch_time = g_globalvars.time;
	h->cfn.hook_status |= HOOK_FRAME_8;
}

void CFN_HookFire(void)
{
	gedict_t *p = self, *h, *link, *e;
	vec3_t angle, delta, origin;
	int n;
	if ((p->cfn.hook_status & HOOK_EXISTS) || p->ct != ctPlayer || p->s.v.deadflag) return;
	p->cfn.hook_status |= HOOK_ACTIVE | HOOK_EXISTS;
	p->cfn.hook_touch_time = g_globalvars.time;
	trap_makevectors(p->s.v.v_angle);
	h = spawn();
	h->classname = "ctfnormal_hook";
	h->s.v.owner = EDICT_TO_PROG(p);
	h->cfn.generation = p->cfn.generation;
	h->s.v.movetype = MOVETYPE_FLY;
	h->isMissile = true;
	h->s.v.solid = SOLID_BBOX;
	VectorScale(g_globalvars.v_forward, 1400, h->s.v.velocity);
	SetVector(h->s.v.avelocity, 300, 300, 300);
	h->touch = (func_t)HookTouch;
	h->s.v.nextthink = g_globalvars.time + 2;
	h->think = (func_t)HookRemove;
	h->cfn.weapon_mode = CFN_WEAPON_HOOK;
	setmodel(h, "progs/v_spike.mdl");
	setsize(h, 0, 0, 0, 0, 0, 0);
	VectorMA(p->s.v.origin, 16, g_globalvars.v_forward, origin); origin[2] += 16;
	setorigin(h, PASSVEC3(origin));
	sound(h, CHAN_WEAPON, "misc/power.wav", 1, ATTN_NORM);
	p->cfn.hook_next = h;
	vectoangles(h->s.v.velocity, angle);
	VectorSubtract(p->s.v.origin, h->s.v.origin, delta); delta[2] += 16;
	link = h;
	for (n = 8; n > 0; --n) {
		e = spawn();
		e->classname = "ctfnormal_hook_chain";
		e->s.v.owner = EDICT_TO_PROG(h);
		e->cfn.hook_xn = n / 9.0f;
		e->s.v.movetype = MOVETYPE_NOCLIP;
		e->s.v.solid = SOLID_NOT;
		VectorCopy(angle, e->s.v.angles);
		e->think = (func_t)HookChainThink;
		e->s.v.nextthink = g_globalvars.time + 0.1f;
		setmodel(e, "progs/s_spike.mdl");
		setsize(e, 0, 0, 0, 0, 0, 0);
		VectorMA(h->s.v.origin, e->cfn.hook_xn, delta, origin);
		setorigin(e, PASSVEC3(origin));
		link->cfn.hook_next = e;
		link = e;
	}
	link->cfn.hook_next = NULL;
}

static void HookLandPlayerThenRemove(void)
{
	if (!HookOwnsPlayer(self)) { ent_remove(self); return; }
	if (self->cfn.hook_status & HOOK_FRAME_8) {
		HookOwner(self)->cfn.hook_status = 0;
		HookOwner(self)->cfn.hook_next = NULL;
		ent_remove(self);
		return;
	}
	HookAnimateReturn();
	self->s.v.nextthink = g_globalvars.time + 0.1f;
}

static void HookRemove(void)
{
	gedict_t *e = self->cfn.hook_next, *next;
	while (e && e != world) { next = e->cfn.hook_next; ent_remove(e); e = next; }
	self->cfn.hook_next = NULL;
	if (!HookOwnsPlayer(self)) { ent_remove(self); return; }
	if (!(self->cfn.hook_status & HOOK_FRAME_8)) {
		self->think = (func_t)HookLandPlayerThenRemove;
		self->touch = (func_t)SUB_Null;
		setmodel(self, "");
		HookLandPlayerThenRemove();
		return;
	}
	HookOwner(self)->cfn.hook_status = 0;
	HookOwner(self)->cfn.hook_next = NULL;
	ent_remove(self);
}

void CFN_HookDestroy(gedict_t *p)
{
	p->cfn.hook_status &= ~HOOK_ACTIVE;
	if (p->cfn.hook_next && p->cfn.hook_next != world)
		p->cfn.hook_next->s.v.nextthink = g_globalvars.time + 0.0001f;
}

void CFN_HookFrame(void)
{
	/* Movement itself is the original 0.1-second entity thinker. This guard
 * only releases dead, teleporting or no-longer-enabled players. */
	if (self->cfn.hook_next && (!CFN_Enabled() || self->s.v.deadflag
		|| self->teleport_time > g_globalvars.time)) CFN_HookDestroy(self);
}
