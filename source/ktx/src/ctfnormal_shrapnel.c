/*
 * Shrapnel 1.0, Copyright (C) 1996, 1997 Johannes Plass.
 * Native KTX port of ctfnormal/src/_shrap.qc.
 * GPL version 2 or (at your option) any later version.
 * Original author's contact: plass@dipmza.physik.uni-mainz.de
 */
#include "g_local.h"
#include "ctfnormal.h"

#define CFN_SHRAPNEL_SPEED 600.0f
#define CFN_SHRAPNEL_RADIUS 70.0f

void spawn_touchblood(float damage);

static void CFN_ShrapnelMissileTouch(void);
static void CFN_ShrapnelDebrisTouch(void);
static void CFN_ShrapnelDebrisThink(void);

static gedict_t *CFN_ShrapnelOwner(gedict_t *projectile)
{
	gedict_t *owner = projectile->cfn.shrapnel_owner;

	if (!owner)
		return world;
	/* Respawn keeps credit; a replacement client in the same slot does not. */
	if (owner != world && NUM_FOR_EDICT(owner) <= MAX_CLIENTS
		&& (!streq(owner->classname, "player") || owner->connect_time != projectile->connect_time))
		return world;
	return owner;
}

void CFN_ShrapnelPrecache(void)
{
	trap_precache_sound("weapons/sgun1.wav");
	trap_precache_sound("knight/sword1.wav");
	trap_precache_model("progs/grenade.mdl");
	trap_precache_model("progs/flame2.mdl");
	trap_precache_model("progs/s_explod.spr");
}

void CFN_ShrapnelFire(void)
{
	gedict_t *proj, *flame;
	vec3_t origin;

	self->s.v.currentammo = self->s.v.ammo_rockets = self->s.v.ammo_rockets - 1;
	/* QuakeWorld represents the original punchangle_x=-2 as SMALLKICK. */
	g_globalvars.msg_entity = EDICT_TO_PROG(self);
	WriteByte(MSG_ONE, SVC_SMALLKICK);
	trap_makevectors(self->s.v.v_angle);

	proj = spawn();
	proj->classname = "shrapnel_projectile";
	proj->s.v.owner = EDICT_TO_PROG(self);
	proj->cfn.shrapnel_owner = self;
	proj->connect_time = self->connect_time;
	proj->s.v.movetype = MOVETYPE_FLYMISSILE;
	proj->isMissile = true;
	proj->s.v.solid = SOLID_BBOX;
	aim(proj->s.v.velocity);
	VectorScale(proj->s.v.velocity, 850, proj->s.v.velocity);
	vectoangles(proj->s.v.velocity, proj->s.v.angles);
	proj->s.v.angles[0] += 180;
	proj->touch = (func_t)CFN_ShrapnelMissileTouch;
	proj->think = (func_t)SUB_Remove;
	proj->s.v.nextthink = proj->s.v.ltime = g_globalvars.time + 6;
	proj->cfn.weapon_mode = CFN_WEAPON_SHRAPNEL;
	setmodel(proj, "progs/grenade.mdl");
	setsize(proj, 0, 0, 0, 0, 0, 0);
	VectorMA(self->s.v.origin, 36, g_globalvars.v_forward, origin);
	origin[2] += 14;
	setorigin(proj, PASSVEC3(origin));

	flame = spawn();
	flame->classname = "shrapnel_flame";
	flame->s.v.owner = proj->s.v.owner;
	VectorCopy(proj->s.v.velocity, flame->s.v.velocity);
	flame->s.v.movetype = proj->s.v.movetype;
	flame->isMissile = true;
	flame->s.v.solid = proj->s.v.solid;
	flame->think = flame->touch = (func_t)SUB_Remove;
	flame->s.v.nextthink = flame->s.v.ltime = proj->s.v.ltime;
	vectoangles(flame->s.v.velocity, flame->s.v.angles);
	flame->s.v.angles[0] += 90;
	flame->s.v.frame = 1;
	setmodel(flame, "progs/flame2.mdl");
	setsize(flame, 0, 0, 0, 0, 0, 0);
	VectorMA(self->s.v.origin, 18, g_globalvars.v_forward, origin);
	origin[2] += 14;
	setorigin(flame, PASSVEC3(origin));
	sound(self, CHAN_WEAPON, "weapons/sgun1.wav", 0.7, ATTN_NORM);
	sound(proj, CHAN_BODY, "knight/sword1.wav", 1, ATTN_NORM);
}

static void CFN_ShrapnelBounceDirection(void)
{
	vec3_t direction, start, end, reflected;
	float dot;

	normalize(self->s.v.velocity, direction);
	VectorSubtract(self->s.v.origin, direction, start);
	VectorAdd(self->s.v.origin, direction, end);
	traceline(PASSVEC3(start), PASSVEC3(end), true, self);
	dot = DotProduct(self->s.v.velocity, g_globalvars.trace_plane_normal);
	VectorMA(self->s.v.velocity, -2 * dot, g_globalvars.trace_plane_normal, reflected);
	normalize(reflected, g_globalvars.v_forward);
	/* This inversion is present in the original; preserve its bounce behavior. */
	g_globalvars.v_forward[2] *= -1;
}

static gedict_t *CFN_ShrapnelDebrisFire(vec3_t velocity)
{
	gedict_t *debris = spawn();

	VectorCopy(self->s.v.origin, debris->s.v.origin);
	debris->classname = "shrapnel_debris";
	debris->s.v.owner = EDICT_TO_PROG(world);
	debris->cfn.shrapnel_owner = self->cfn.shrapnel_owner;
	debris->connect_time = self->connect_time;
	VectorScale(velocity, CFN_SHRAPNEL_SPEED, debris->s.v.velocity);
	vectoangles(debris->s.v.velocity, debris->s.v.angles);
	debris->s.v.angles[0] += 90;
	debris->s.v.movetype = MOVETYPE_FLYMISSILE;
	debris->isMissile = true;
	debris->s.v.solid = SOLID_BBOX;
	debris->think = (func_t)CFN_ShrapnelDebrisThink;
	debris->touch = (func_t)CFN_ShrapnelDebrisTouch;
	debris->s.v.ltime = g_globalvars.time + 0.7 + g_random() * 0.3;
	debris->s.v.nextthink = debris->s.v.ltime + 2;
	debris->cfn.weapon_mode = CFN_WEAPON_SHRAPNEL;
	debris->s.v.effects = EF_DIMLIGHT;
	setmodel(debris, "progs/flame2.mdl");
	setsize(debris, 0, 0, 0, 0, 0, 0);
	setorigin(debris, PASSVEC3(debris->s.v.origin));
	return debris;
}

/* The original uses combat.qc's radius rule, not fragment falloff below. */
static void CFN_ShrapnelMissileRadius(float damage)
{
	gedict_t *head = world;
	gedict_t *attacker = CFN_ShrapnelOwner(self);
	vec3_t delta;
	float points;
	int axis;

	while ((head = trap_findradius(head, self->s.v.origin, damage + 40)))
	{
		if (!head->s.v.takedamage)
			continue;
		for (axis = 0; axis < 3; ++axis)
			delta[axis] = self->s.v.origin[axis] - head->s.v.origin[axis]
				- (head->s.v.mins[axis] + head->s.v.maxs[axis]) * 0.5;
		points = damage - 0.5 * vlen(delta);
		if (head == attacker)
			points *= 0.5;
		if (points > 0 && CanDamage(head, self))
		{
			if (streq(head->classname, "monster_shambler"))
				points *= 0.5;
			CFN_Damage(head, self, attacker, points, CFN_WEAPON_SHRAPNEL);
		}
	}
}

static void CFN_ShrapnelMissileExplosionThink(void)
{
	self->s.v.frame += 1;
	self->s.v.nextthink = g_globalvars.time + 0.1;
	self->think = (func_t)(self->s.v.frame >= 5 ? SUB_Remove : CFN_ShrapnelMissileExplosionThink);
}

static void CFN_ShrapnelMissileTouch(void)
{
	float r = 0, up, right;
	vec3_t direction, angles, delta;
	int i, fragments;

	if (trap_pointcontents(PASSVEC3(self->s.v.origin)) == CONTENT_SKY)
	{
		ent_remove(self);
		return;
	}
	sound(self, CHAN_WEAPON, "weapons/sgun1.wav", 0.7, ATTN_NORM);
	if (other->s.v.solid == SOLID_BSP)
	{
		r = g_random();
		CFN_ShrapnelMissileRadius(30 + r * 10);
		if (r > 0.5)
			CFN_BurnSetOnFire(other, CFN_ShrapnelOwner(self));
	}
	CFN_ShrapnelBounceDirection();
	vectoangles(g_globalvars.v_forward, angles);
	trap_makevectors(angles);
	CFN_ShrapnelDebrisFire(g_globalvars.v_forward);
	fragments = r > 0.5 ? 3 : 2;
	for (i = 0; i < fragments; ++i)
	{
		up = 1.5 * (0.5 - g_random());
		right = 1.5 * (0.5 - g_random());
		VectorMA(g_globalvars.v_forward, up, g_globalvars.v_up, direction);
		VectorMA(direction, right, g_globalvars.v_right, direction);
		normalize(direction, direction);
		CFN_ShrapnelDebrisFire(direction);
	}
	normalize(self->s.v.velocity, delta);
	VectorMA(self->s.v.origin, -4, delta, self->s.v.origin);
	self->s.v.movetype = MOVETYPE_NONE;
	VectorClear(self->s.v.velocity);
	self->touch = (func_t)SUB_Null;
	setmodel(self, "progs/s_explod.spr");
	self->s.v.solid = SOLID_NOT;
	self->s.v.frame = 0;
	self->think = (func_t)CFN_ShrapnelMissileExplosionThink;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

static void CFN_ShrapnelDebrisExplode3(void)
{
	self->s.v.frame = 4;
	self->think = (func_t)SUB_Remove;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

static void CFN_ShrapnelDebrisExplode2(void)
{
	self->s.v.frame = 3;
	self->think = (func_t)CFN_ShrapnelDebrisExplode3;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

static void CFN_ShrapnelDebrisExplode(qbool do_damage)
{
	gedict_t *e = world;
	float r, damage, base;
	vec3_t delta;

	if (do_damage)
	{
		self->s.v.owner = EDICT_TO_PROG(CFN_ShrapnelOwner(self));
		if (other->s.v.takedamage)
			spawn_touchblood(9);
		while ((e = trap_findradius(e, self->s.v.origin, CFN_SHRAPNEL_RADIUS)))
		{
			if (!e->s.v.takedamage)
				continue;
			if (e->s.v.takedamage != DAMAGE_AIM)
			{
				CFN_Damage(e, self, PROG_TO_EDICT(self->s.v.owner), 10, CFN_WEAPON_SHRAPNEL);
				continue;
			}
			r = g_random();
			base = 25 + (0.5 - r) * 5;
			VectorSubtract(e->s.v.origin, self->s.v.origin, delta);
			delta[2] += 16;
			damage = vlen(delta);
			if (damage < CFN_SHRAPNEL_RADIUS - 2 * base)
				damage = base;
			else if (damage < CFN_SHRAPNEL_RADIUS)
				damage = (CFN_SHRAPNEL_RADIUS - damage) * 0.5;
			else
				damage = 0;
			if (damage)
			{
				CFN_Damage(e, self, PROG_TO_EDICT(self->s.v.owner), damage, CFN_WEAPON_SHRAPNEL);
				if (r > 0.66 && damage > 6)
					CFN_BurnSetOnFire(e, PROG_TO_EDICT(self->s.v.owner));
			}
		}
		self->s.v.owner = EDICT_TO_PROG(world);
	}
	self->classname = "shrapnel_explosion";
	normalize(self->s.v.velocity, delta);
	VectorMA(self->s.v.origin, -4, delta, self->s.v.origin);
	self->s.v.movetype = MOVETYPE_NONE;
	VectorClear(self->s.v.velocity);
	self->touch = (func_t)SUB_Null;
	self->s.v.solid = SOLID_NOT;
	self->think = (func_t)SUB_Remove;
	self->s.v.nextthink = g_globalvars.time + 4;
	setmodel(self, "progs/s_explod.spr");
	setorigin(self, PASSVEC3(self->s.v.origin));
	self->s.v.frame = 0;
	self->think = (func_t)CFN_ShrapnelDebrisExplode2;
	self->s.v.nextthink = g_globalvars.time + 0.1;
}

static void CFN_ShrapnelDebrisTouch(void)
{
	qbool do_damage = false, play_sound = false;
	gedict_t *e, *saved_self;
	float dot;

	if (trap_pointcontents(PASSVEC3(self->s.v.origin)) == CONTENT_SKY)
	{
		ent_remove(self);
		return;
	}
	if (g_globalvars.time > self->cfn.shrapnel_bounce_time + 0.1)
	{
		CFN_ShrapnelBounceDirection();
		self->s.v.movetype = MOVETYPE_BOUNCE;
		dot = DotProduct(g_globalvars.v_forward, self->s.v.velocity);
		VectorMA(self->s.v.velocity, dot, g_globalvars.v_forward, self->s.v.velocity);
		self->s.v.nextthink = g_globalvars.time;
		vectoangles(self->s.v.velocity, self->s.v.angles);
		self->s.v.angles[0] += 90;
	}
	else
		self->s.v.ltime = g_globalvars.time;
	if (other->s.v.solid != SOLID_BSP)
	{
		e = self;
		do_damage = play_sound = true;
	}
	else if (g_globalvars.time < self->s.v.ltime)
	{
		if (g_globalvars.time > self->cfn.shrapnel_damage_time + 0.1)
		{
			do_damage = true;
			self->cfn.shrapnel_damage_time = g_globalvars.time;
		}
		if (g_globalvars.time > self->cfn.shrapnel_sound_time + 0.1)
		{
			self->cfn.shrapnel_sound_time = g_globalvars.time;
			play_sound = true;
		}
		e = spawn();
		VectorCopy(self->s.v.origin, e->s.v.origin);
		VectorCopy(self->s.v.velocity, e->s.v.velocity);
		e->cfn.shrapnel_owner = self->cfn.shrapnel_owner;
		e->connect_time = self->connect_time;
		e->cfn.weapon_mode = CFN_WEAPON_SHRAPNEL;
		e->s.v.effects = EF_DIMLIGHT;
		setsize(e, 0, 0, 0, 0, 0, 0);
	}
	else
	{
		e = self;
		do_damage = play_sound = true;
	}
	saved_self = self;
	self = e;
	if (play_sound)
		sound(self, CHAN_WEAPON, "weapons/sgun1.wav", 0.6, ATTN_NORM);
	CFN_ShrapnelDebrisExplode(do_damage);
	self = saved_self;
}

static void CFN_ShrapnelDebrisThink(void)
{
	gedict_t *saved_other;

	if (g_globalvars.time < self->s.v.ltime + 2 && vlen(self->s.v.velocity) > 100)
	{
		self->s.v.flags = (int)self->s.v.flags & ~FL_ONGROUND;
		normalize(self->s.v.velocity, self->s.v.velocity);
		VectorScale(self->s.v.velocity, CFN_SHRAPNEL_SPEED, self->s.v.velocity);
		self->s.v.movetype = MOVETYPE_FLYMISSILE;
		self->s.v.nextthink = self->s.v.ltime + 2;
		vectoangles(self->s.v.velocity, self->s.v.angles);
		self->s.v.angles[0] += 90;
		return;
	}
	saved_other = other;
	CFN_ShrapnelDebrisExplode(true);
	other = saved_other;
}
