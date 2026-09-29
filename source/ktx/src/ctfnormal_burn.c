/*
 * Burn, ported from CTFNormal _burn.qc (Burn Code 1.0).
 * Copyright (C) 1996, 1997 Johannes Plass <plass@dipmza.physik.uni-mainz.de>
 * GPL-2.0-or-later; see COPYING. Original behavior and constants retained.
 */
#include "g_local.h"

void bubble_bob(void);
void barrel_explode(void);

static void CFN_BurnMakeBubble(void)
{
	gedict_t *bubble;
	gedict_t *owner = PROG_TO_EDICT(self->s.v.owner);
	vec3_t origin;

	if (owner != world && NUM_FOR_EDICT(owner) <= MAX_CLIENTS
		&& (!streq(owner->classname, "player") || owner->connect_time != self->connect_time))
	{
		ent_remove(self);
		return;
	}
	bubble = spawn();

	bubble->s.v.movetype = MOVETYPE_NOCLIP;
	bubble->s.v.solid = SOLID_NOT;
	SetVector(bubble->s.v.velocity, 0, 0, 15);
	bubble->s.v.nextthink = g_globalvars.time + 0.5;
	bubble->think = (func_t) bubble_bob;
	bubble->classname = "bubble";
	bubble->s.v.owner = self->s.v.owner;
	bubble->s.v.frame = 0;
	bubble->cnt = 0;
	setmodel(bubble, "progs/s_bubble.spr");
	VectorCopy(owner->s.v.origin, origin);
	origin[2] += 24;
	setorigin(bubble, PASSVEC3(origin));
	setsize(bubble, -8, -8, -8, 8, 8, 8);

	self->s.v.nextthink = g_globalvars.time + 0.1 + g_random() * 0.2;
	self->think = (func_t) CFN_BurnMakeBubble;
	self->air_finished += 1;
	if (self->air_finished >= self->bubble_count)
	{
		ent_remove(self);
	}
}

static void CFN_BurnSteam(gedict_t *victim, int num_bubbles)
{
	gedict_t *steam = spawn();
	steam->classname = "burn_steam";
	steam->s.v.movetype = MOVETYPE_NONE;
	steam->s.v.solid = SOLID_NOT;
	steam->s.v.nextthink = g_globalvars.time + 0.1;
	steam->think = (func_t) CFN_BurnMakeBubble;
	steam->air_finished = 0;
	steam->s.v.owner = EDICT_TO_PROG(victim);
	steam->connect_time = victim->connect_time;
	steam->bubble_count = num_bubbles;
	setorigin(steam, PASSVEC3(victim->s.v.origin));
	sound(victim, CHAN_BODY, "player/slimbrn2.wav", 1, ATTN_NORM);
}

static void CFN_BurnExplodeThink(void)
{
	if (self->s.v.frame >= 5)
	{
		ent_remove(self);
		return;
	}
	self->s.v.frame += 1;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

static void CFN_BurnRemoveFlames(gedict_t *victim, gedict_t *flame)
{
	gedict_t *flame2 = flame->cfn.burn_flame;
	if (victim->cfn.burn_flame == flame)
	{
		victim->cfn.burn_flame = NULL;
		victim->cfn.burn_flame2 = NULL;
		victim->cfn.burn_burning = 0;
	}
	if (flame2 && (flame2 != world) && streq(flame2->classname, "burn_flame2"))
	{
		ent_remove(flame2);
	}
	flame->cfn.burn_flame = NULL;
}

static void CFN_BurnThink(void)
{
	gedict_t *flame = self;
	gedict_t *victim = PROG_TO_EDICT(flame->s.v.enemy);
	gedict_t *owner = PROG_TO_EDICT(flame->s.v.owner);
	gedict_t *e;
	float damage, r, forward_jitter, right_jitter;
	vec3_t center, offset, primary, secondary;
	int burning;

	/* Keep credit through respawns, but never give it to a replacement client. */
	if (owner != world && NUM_FOR_EDICT(owner) <= MAX_CLIENTS
		&& (!streq(owner->classname, "player") || owner->connect_time != flame->connect_time))
	{
		owner = world;
	}

	/* Respawn/disconnect cleanup invalidates ownership before the next tick. */
	if (victim->cfn.burn_flame != flame)
	{
		CFN_BurnRemoveFlames(victim, flame);
		ent_remove(flame);
		return;
	}
	if (victim->s.v.deadflag != DEAD_NO && victim->cfn.burn_burning)
	{
		if (victim->cfn.killweapon != CFN_WEAPON_BURN)
		{
			victim->cfn.burn_burning = 0;
		}
		else if (victim->s.v.deadflag == DEAD_DEAD)
		{
			CFN_BurnRemoveFlames(victim, flame);
			VectorCopy(flame->s.v.origin, primary);
			primary[2] -= 12;
			setorigin(flame, PASSVEC3(primary));
			flame->s.v.movetype = MOVETYPE_NONE;
			VectorClear(flame->s.v.velocity);
			flame->touch = (func_t) SUB_Null;
			flame->s.v.solid = SOLID_NOT;
			setmodel(flame, "progs/s_explod.spr");
			flame->s.v.frame = 0;
			flame->think = (func_t) CFN_BurnExplodeThink;
			flame->s.v.nextthink = g_globalvars.time + 0.1;
			return;
		}
	}

	if (g_globalvars.time > flame->cfn.burn_damage_time)
	{
		if (victim->s.v.waterlevel > 1)
		{
			CFN_BurnSteam(victim, 8);
			victim->cfn.burn_burning = 0;
		}
		if (victim->cfn.burn_burning)
		{
			damage = 0;
			burning = (int) victim->cfn.burn_burning;
			if (burning & 1)
			{
				damage += 3;
				if (g_globalvars.time > flame->cfn.burn_lifetime1) burning &= ~1;
			}
			if (burning & 2)
			{
				damage += 3;
				if (g_globalvars.time > flame->cfn.burn_lifetime2) burning &= ~2;
			}
			if (burning & 4)
			{
				damage += 3;
				if (g_globalvars.time > flame->cfn.burn_lifetime4) burning &= ~4;
			}
			victim->cfn.burn_burning = burning;
			r = g_random();
			if ((damage > 0) && (victim->s.v.health > 0))
			{
				CFN_Damage(victim, flame, owner, damage, CFN_WEAPON_BURN);
			}
			VectorCopy(victim->s.v.origin, center);
			center[2] += 18;
			for (e = trap_findradius(world, center, 50); e; e = trap_findradius(e, center, 50))
			{
				if ((e != victim) && (e->s.v.takedamage == DAMAGE_AIM))
				{
					CFN_Damage(e, flame, victim, 6 + r * 4, CFN_WEAPON_BURN);
					if (r > 0.5)
					{
						CFN_BurnSetOnFire(e, victim);
					}
				}
			}
		}
		flame->cfn.burn_damage_time = g_globalvars.time + 1;
	}

	if (victim->cfn.burn_burning)
	{
		trap_makevectors(victim->s.v.v_angle);
		forward_jitter = crandom() * 2;
		right_jitter = crandom() * 4;
		VectorScale(g_globalvars.v_forward, forward_jitter, offset);
		VectorMA(offset, right_jitter, g_globalvars.v_right, offset);
		VectorMA(victim->s.v.origin, -7, g_globalvars.v_forward, center);
		center[2] += (victim->s.v.deadflag != DEAD_NO) ? 6 : 18;
		VectorAdd(center, offset, primary);
		VectorSubtract(center, offset, secondary);
		setorigin(flame, PASSVEC3(primary));
		setorigin(flame->cfn.burn_flame, PASSVEC3(secondary));
		flame->s.v.nextthink = g_globalvars.time + 0.02;
		return;
	}
	CFN_BurnRemoveFlames(victim, flame);
	ent_remove(flame);
}

void CFN_BurnPrecache(void)
{
	trap_precache_model("progs/flame2.mdl");
	trap_precache_model("progs/s_bubble.spr");
	trap_precache_model("progs/s_explod.spr");
	trap_precache_sound("player/lburn1.wav");
	trap_precache_sound("player/lburn2.wav");
	trap_precache_sound("player/slimbrn2.wav");
	trap_precache_sound("boss1/throw.wav");
}

void CFN_BurnSetOnFire(gedict_t *victim, gedict_t *attacker)
{
	gedict_t *flame, *flame2;
	vec3_t origin;
	int burning;

	if (!CFN_Enabled() || (victim->s.v.waterlevel > 1)
		|| (victim->invincible_finished >= g_globalvars.time)
		|| streq(victim->classname, "drone") || (victim->th_die == barrel_explode)
		|| (victim->s.v.health <= 0))
	{
		return;
	}
	/* CTFNormal's separate Protect module is disabled by its active config. */
	if (isCTF() && victim != attacker && SameTeam(victim, attacker))
	{
		return;
	}
	if (!victim->cfn.burn_burning)
	{
		flame = spawn();
		flame->s.v.owner = EDICT_TO_PROG(attacker);
		flame->connect_time = attacker->connect_time;
		flame->s.v.enemy = EDICT_TO_PROG(victim);
		flame->s.v.movetype = MOVETYPE_NONE;
		VectorClear(flame->s.v.velocity);
		flame->s.v.solid = SOLID_NOT;
		flame->s.v.ltime = g_globalvars.time + 10;
		flame->classname = "burn_flame";
		flame->think = (func_t) CFN_BurnThink;
		flame->s.v.nextthink = g_globalvars.time + 0.1;
		flame->cfn.weapon_mode = CFN_WEAPON_BURN;
		flame->s.v.effects = (int) flame->s.v.effects | EF_DIMLIGHT;
		setmodel(flame, "progs/flame2.mdl");
		flame->s.v.frame = 1;
		setsize(flame, 0, 0, 0, 0, 0, 0);
		VectorCopy(victim->s.v.origin, origin);
		origin[0] -= 4;
		origin[2] += 18;
		setorigin(flame, PASSVEC3(origin));
		victim->cfn.burn_flame = flame;

		flame2 = spawn();
		flame2->s.v.owner = EDICT_TO_PROG(attacker);
		flame2->s.v.enemy = EDICT_TO_PROG(victim);
		flame2->s.v.movetype = MOVETYPE_NONE;
		VectorClear(flame2->s.v.velocity);
		flame2->s.v.solid = SOLID_NOT;
		flame2->s.v.ltime = g_globalvars.time + 15;
		flame2->classname = "burn_flame2";
		flame2->think = (func_t) SUB_Null;
		setmodel(flame2, "progs/flame2.mdl");
		flame2->s.v.frame = 1;
		setsize(flame2, 0, 0, 0, 0, 0, 0);
		origin[0] += 8;
		setorigin(flame2, PASSVEC3(origin));
		victim->cfn.burn_flame2 = flame2;
		flame->cfn.burn_flame = flame2;
	}
	else
	{
		flame = victim->cfn.burn_flame;
	}

	burning = (int) victim->cfn.burn_burning;
	if (burning == 7)
	{
		/* Preserve the original nested refresh comparisons, including ties. */
		if (flame->cfn.burn_lifetime1 <= flame->cfn.burn_lifetime2)
		{
			if (flame->cfn.burn_lifetime1 <= flame->cfn.burn_lifetime4)
				flame->cfn.burn_lifetime1 = g_globalvars.time + 15;
		}
		else if (flame->cfn.burn_lifetime2 <= flame->cfn.burn_lifetime4)
		{
			if (flame->cfn.burn_lifetime2 <= flame->cfn.burn_lifetime1)
				flame->cfn.burn_lifetime2 = g_globalvars.time + 15;
		}
		else
		{
			flame->cfn.burn_lifetime4 = g_globalvars.time + 15;
		}
	}
	else if (!(burning & 1))
	{
		flame->cfn.burn_lifetime1 = g_globalvars.time + 15;
		victim->cfn.burn_burning = burning | 1;
	}
	else if (!(burning & 2))
	{
		flame->cfn.burn_lifetime2 = g_globalvars.time + 15;
		victim->cfn.burn_burning = burning | 2;
	}
	else
	{
		flame->cfn.burn_lifetime4 = g_globalvars.time + 15;
		victim->cfn.burn_burning = burning | 4;
	}
	sound(flame, CHAN_WEAPON, "boss1/throw.wav", 1, ATTN_NORM);
}

void CFN_BurnPainSound(void)
{
	if (g_globalvars.time < self->cfn.burn_painsound_time)
	{
		return;
	}
	sound(self, CHAN_VOICE, (g_random() > 0.5) ? "player/lburn1.wav" : "player/lburn2.wav",
		1, ATTN_NORM);
	self->cfn.burn_painsound_time = g_globalvars.time + 0.8;
}

void CFN_BurnCleanup(gedict_t *p)
{
	gedict_t *flame = p->cfn.burn_flame;
	if (flame && flame != world && streq(flame->classname, "burn_flame")
		&& PROG_TO_EDICT(flame->s.v.enemy) == p)
	{
		CFN_BurnRemoveFlames(p, flame);
		ent_remove(flame);
	}
	p->cfn.burn_flame = NULL;
	p->cfn.burn_flame2 = NULL;
	p->cfn.burn_burning = 0;
	p->cfn.burn_painsound_time = 0;
}
