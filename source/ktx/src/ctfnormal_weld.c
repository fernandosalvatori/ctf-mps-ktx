/*
 * WeldGun, ported from CTFNormal _weldgun.qc (WeldGun Code 1.0).
 * Copyright (C) 1996, 1997 Johannes Plass <plass@dipmza.physik.uni-mainz.de>
 * GPL-2.0-or-later; see COPYING. Original behavior and constants retained.
 */
#include "g_local.h"

void spawn_touchblood(float damage);

static void CFN_WeldExplode3(void)
{
	self->s.v.frame = 4;
	self->think = (func_t) SUB_Remove;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

static void CFN_WeldExplode2(void)
{
	self->s.v.frame = 3;
	self->think = (func_t) CFN_WeldExplode3;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

static void CFN_WeldTouch(void)
{
	gedict_t *e;
	gedict_t *blob = self;
	gedict_t *owner = PROG_TO_EDICT(blob->s.v.owner);
	vec3_t delta, origin;
	float r, damage, base_damage, distance;

	/* A disconnected client's slot can be reused before this projectile expires. */
	if (owner != world && NUM_FOR_EDICT(owner) <= MAX_CLIENTS
		&& (!streq(owner->classname, "player") || owner->connect_time != blob->connect_time))
	{
		owner = world;
	}

	if (trap_pointcontents(PASSVEC3(blob->s.v.origin)) == CONTENT_SKY)
	{
		ent_remove(blob);
		return;
	}

	if (other->s.v.takedamage)
	{
		spawn_touchblood(9);
	}

	/* QC findradius used a linked chain; QW iterates the same radius by edict. */
	for (e = trap_findradius(world, blob->s.v.origin, 60); e;
		 e = trap_findradius(e, blob->s.v.origin, 60))
	{
		if (!e->s.v.takedamage)
		{
			continue;
		}
		if (e->s.v.takedamage != DAMAGE_AIM)
		{
			CFN_Damage(e, blob, owner, 10, CFN_WEAPON_WELD);
			continue;
		}
		r = g_random();
		base_damage = 11 + (0.5 - r) * 6;
		VectorSubtract(e->s.v.origin, blob->s.v.origin, delta);
		delta[2] += 16;
		distance = vlen(delta);
		if (distance < 3 * base_damage)
		{
			damage = base_damage;
		}
		else if (distance < 60)
		{
			damage = base_damage * (60 - distance) / (60 - 3 * base_damage);
		}
		else
		{
			damage = 0;
		}
		if (damage)
		{
			CFN_Damage(e, blob, owner, damage, CFN_WEAPON_WELD);
			if ((r > 0.85f) && (damage > 5))
			{
				CFN_BurnSetOnFire(e, owner);
			}
		}
	}

	normalize(blob->s.v.velocity, delta);
	VectorMA(blob->s.v.origin, -4, delta, origin);
	blob->s.v.movetype = MOVETYPE_NONE;
	VectorClear(blob->s.v.velocity);
	blob->touch = (func_t) SUB_Null;
	blob->s.v.solid = SOLID_NOT;
	setmodel(blob, "progs/s_explod.spr");
	setorigin(blob, PASSVEC3(origin));
	sound(blob, CHAN_BODY, "wizard/hit.wav", 1, ATTN_NORM);
	blob->s.v.frame = 0;
	blob->think = (func_t) CFN_WeldExplode2;
	blob->s.v.nextthink = g_globalvars.time + 0.1;
}

void CFN_WeldPrecache(void)
{
	trap_precache_model("progs/flame2.mdl");
	trap_precache_model("progs/s_explod.spr");
	trap_precache_sound("weapons/spike2.wav");
	trap_precache_sound("hknight/idle.wav");
	trap_precache_sound("wizard/hit.wav");
}

void CFN_WeldFire(vec3_t org, vec3_t dir)
{
	gedict_t *weld = spawn();
	vec3_t origin;

	weld->s.v.owner = EDICT_TO_PROG(self);
	weld->connect_time = self->connect_time;
	weld->s.v.movetype = MOVETYPE_FLYMISSILE;
	weld->s.v.solid = SOLID_BBOX;
	weld->touch = (func_t) CFN_WeldTouch;
	weld->classname = "weld_blob";
	weld->think = (func_t) SUB_Remove;
	weld->s.v.nextthink = g_globalvars.time + 6;
	VectorScale(dir, 1400, weld->s.v.velocity);
	vectoangles(weld->s.v.velocity, weld->s.v.angles);
	weld->s.v.angles[0] += 90;
	weld->cfn.weapon_mode = CFN_WEAPON_WELD;
	if (g_globalvars.time >= self->cfn.weld_light_time)
	{
		weld->s.v.effects = (int) weld->s.v.effects | EF_DIMLIGHT;
		self->cfn.weld_light_time = g_globalvars.time + 0.2;
	}
	setmodel(weld, "progs/flame2.mdl");
	setsize(weld, 0, 0, 0, 0, 0, 0);
	VectorMA(org, 8, dir, origin);
	origin[2] -= 6;
	setorigin(weld, PASSVEC3(origin));
	sound(self, CHAN_WEAPON, "weapons/spike2.wav", 0.6, ATTN_NORM);
	sound(weld, CHAN_BODY, "hknight/idle.wav", 1, ATTN_NORM);
}
