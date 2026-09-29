/* ServerModules Drone 1.0, Copyright (C) 1996, 1997 Johannes Plass.
 * Port of ctfnormal/src/_drone.qc and _drone.qh to KTX.
 * GPL version 2 or (at your option) any later version. See COPYING.
 */
#include "g_local.h"
#include "ctfnormal.h"
#include <stddef.h>
extern void AmmoUsed(gedict_t *player);

#define DRONE_THINK_NEXTTIME 0.2f
#define DRONE_THINK_FIRSTTIME 0.6f
#define DRONE_LIFETIME 6.0f
#define DRONE_VELOCITY_START 400.0f
#define DRONE_TARGET_OFFSET_Z 16.0f
#define DRONE_DAMAGE_RADIUS 70.0f

static void DroneThink(void);
static void DroneTouch(void);
static void DroneDie(void);
static int DroneFindTarget(gedict_t *drone);

static gedict_t *DroneEnemy(gedict_t *d) { return PROG_TO_EDICT(d->s.v.enemy); }

void CFN_DronePrecache(void)
{
	trap_precache_sound("shalrath/attack2.wav");
	trap_precache_sound("knight/khurt.wav");
	trap_precache_sound("hknight/hit.wav");
	trap_precache_sound("weapons/bounce.wav");
	trap_precache_model("progs/lavaball.mdl");
	trap_precache_model("progs/s_explod.spr");
}

static void DroneExplosionFrame(void)
{
	if (++self->s.v.frame >= 6) { ent_remove(self); return; }
	self->s.v.nextthink = g_globalvars.time + 0.1f;
}

static void DroneThreat(gedict_t *p, int threat)
{
	if (!p || p == world) return;
	p->cfn.drone_threat = threat != 0;
}

static void DroneRemoveFromQueue(gedict_t *p, gedict_t *d)
{
	gedict_t **slots[4];
	int i;
	if (!p || !d->cfn.drone_in_queue) return;
	slots[0] = &p->cfn.drone_1; slots[1] = &p->cfn.drone_2;
	slots[2] = &p->cfn.drone_4; slots[3] = &p->cfn.drone_8;
	for (i = 0; i < 4; ++i) if (*slots[i] == d) {
		*slots[i] = NULL;
		p->cfn.drone_count = (int)p->cfn.drone_count & ~(1 << i);
		break;
	}
	d->cfn.drone_in_queue = 0;
}

static void DroneAddToQueue(gedict_t *p, gedict_t *d)
{
	gedict_t **slots[4] = {&p->cfn.drone_1, &p->cfn.drone_2,
		&p->cfn.drone_4, &p->cfn.drone_8};
	int i;
	for (i = 0; i < 4; ++i) if (!((int)p->cfn.drone_count & (1 << i))) {
		*slots[i] = d;
		p->cfn.drone_count = (int)p->cfn.drone_count | (1 << i);
		d->cfn.drone_in_queue = 1;
		return;
	}
}

static void DroneRemoveOldest(gedict_t *p)
{
	gedict_t *slots[4] = {p->cfn.drone_1, p->cfn.drone_2, p->cfn.drone_4, p->cfn.drone_8};
	gedict_t *oldest = NULL;
	float earliest = g_globalvars.time + 10;
	int i;
	if (((int)p->cfn.drone_count & 15) != 15) return;
	for (i = 0; i < 4; ++i) if (slots[i] && slots[i]->s.v.ltime < earliest) {
		oldest = slots[i]; earliest = oldest->s.v.ltime;
	}
	if (oldest) {
		DroneRemoveFromQueue(p, oldest);
		oldest->s.v.nextthink = g_globalvars.time + 0.01f;
		oldest->touch = (func_t)SUB_Null;
		oldest->think = (func_t)DroneDie;
	}
}

/* Called before a client slot is recycled, so old projectiles cannot acquire
 * a new player's identity as their owner. Other players' drones stay alive. */
void CFN_DroneCleanup(gedict_t *p)
{
	gedict_t *d = world;
	while ((d = find(d, (int)offsetof(gedict_t, classname), "drone"))) {
		if (d->cfn.drone_owner == p) {
			DroneThreat(DroneEnemy(d), false);
			DroneRemoveFromQueue(p, d);
			ent_remove(d);
		} else if (DroneEnemy(d) == p) {
			d->s.v.enemy = EDICT_TO_PROG(world);
			d->cfn.drone_target_is_fixed = 1;
		}
	}
	p->cfn.drone_count = 0;
	p->cfn.drone_1 = p->cfn.drone_2 = p->cfn.drone_4 = p->cfn.drone_8 = NULL;
}

void CFN_DroneFire(void)
{
	gedict_t *d, *p = self;
	DroneRemoveOldest(p);
	p->s.v.currentammo = --p->s.v.ammo_rockets;
	AmmoUsed(p);
	trap_makevectors(p->s.v.v_angle);
	g_globalvars.msg_entity = EDICT_TO_PROG(p);
	WriteByte(MSG_ONE, SVC_SMALLKICK);
	d = spawn();
	d->classname = "drone";
	d->s.v.owner = EDICT_TO_PROG(p);
	d->cfn.drone_owner = p;
	d->s.v.movetype = MOVETYPE_FLY;
	d->s.v.solid = SOLID_BBOX;
	d->isMissile = true;
	aim(d->s.v.velocity);
	VectorScale(d->s.v.velocity, DRONE_VELOCITY_START, d->s.v.velocity);
	VectorMA(p->s.v.origin, 12, g_globalvars.v_forward, d->s.v.origin);
	d->s.v.origin[2] += 16;
	d->cfn.weapon_mode = CFN_WEAPON_DRONE;
	vectoangles(d->s.v.velocity, d->s.v.angles);
	d->touch = (func_t)DroneTouch;
	d->s.v.ltime = g_globalvars.time;
	d->s.v.nextthink = g_globalvars.time + DRONE_THINK_FIRSTTIME;
	d->think = (func_t)DroneThink;
	SetVector(d->s.v.avelocity, 300, 300, 300);
	d->s.v.effects = (int)d->s.v.effects | EF_DIMLIGHT;
	d->s.v.health = 20;
	d->s.v.takedamage = DAMAGE_AIM;
	d->th_die = DroneDie;
	d->s.v.enemy = EDICT_TO_PROG(p);
	VectorCopy(p->s.v.origin, d->cfn.drone_target_vector);
	d->cfn.drone_target_vector[2] += DRONE_TARGET_OFFSET_Z;
	VectorCopy(p->s.v.velocity, d->cfn.drone_target_velocity);
	d->cfn.drone_set_target_vector_time = g_globalvars.time - 10;
	DroneFindTarget(d);
	DroneAddToQueue(p, d);
	DroneThreat(DroneEnemy(d), true);
	setmodel(d, "progs/lavaball.mdl");
	setsize(d, 0, 0, 0, 0, 0, 0);
	setorigin(d, PASSVEC3(d->s.v.origin));
	sound(p, CHAN_WEAPON, "shalrath/attack2.wav", 1, ATTN_NORM);
	sound(d, CHAN_WEAPON, "knight/khurt.wav", 1, ATTN_NORM);
	d->cfn.drone_sound_time = g_globalvars.time - 0.3f;
}

static void DroneTouch(void)
{
	gedict_t *d = self, *e;
	vec3_t delta;
	float damage, base, roll, distance;
	if (d->voided) return;
	if (trap_pointcontents(PASSVEC3(d->s.v.origin)) == CONTENT_SKY) {
		DroneThreat(DroneEnemy(d), false);
		/* Original omitted this unlink on sky; do not retain a recycled edict. */
		DroneRemoveFromQueue(d->cfn.drone_owner, d);
		ent_remove(d);
		return;
	}
	if (other->s.v.solid == SOLID_BSP) {
		if (checkbottom(d)) {
			if (d->cfn.drone_bounced) d->s.v.origin[2] += 8;
			if (!d->cfn.drone_firstthink) d->s.v.nextthink = g_globalvars.time + 0.1f;
		}
		if (g_globalvars.time > d->cfn.drone_bounce_sound_time + 0.6f) {
			sound(d, CHAN_WEAPON, "weapons/bounce.wav", 0.6f, ATTN_NORM);
			d->cfn.drone_bounce_sound_time = g_globalvars.time;
			d->cfn.drone_sound_time = g_globalvars.time - 0.4f;
			if (d->s.v.health > 0) CFN_Damage(d, world, world, 3, CFN_WEAPON_DRONE);
			if (d->voided) return;
		}
		d->cfn.drone_bounce_time = g_globalvars.time;
		d->cfn.drone_bounced = 1;
		setsize(d, 0, 0, 0, 0, 0, 0);
		return;
	}
	if (other->s.v.solid == SOLID_BBOX) {
		if (streq(other->classname, "drone") && d->cfn.drone_count >= 0) {
			if (g_globalvars.time > d->cfn.drone_bounce_sound_time + 0.6f) {
				sound(d, CHAN_WEAPON, "weapons/bounce.wav", 0.8f, ATTN_NORM);
				d->cfn.drone_bounce_sound_time = d->cfn.drone_sound_time = g_globalvars.time;
				if (d->s.v.health > 0) CFN_Damage(d, world, world,
					d->cfn.drone_owner == other->cfn.drone_owner ? 3 : 6, CFN_WEAPON_DRONE);
			}
			return;
		}
		if (streq(other->classname, "grenade") || streq(other->classname, "spike")) {
			sound(d, CHAN_WEAPON, "weapons/bounce.wav", 0.6f, ATTN_NORM);
			d->cfn.drone_sound_time = g_globalvars.time;
			if (d->s.v.health > 0) CFN_Damage(d, world, world,
				streq(other->classname, "grenade") ? 5 : 6, CFN_WEAPON_DRONE);
			return;
		}
	}
	d->voided = 1;
	d->think = (func_t)DroneDie;
	d->s.v.nextthink = g_globalvars.time + 3;
	d->s.v.owner = EDICT_TO_PROG(d->cfn.drone_owner);
	DroneThreat(DroneEnemy(d), false);
	DroneRemoveFromQueue(d->cfn.drone_owner, d);
	e = world;
	while ((e = trap_findradius(e, d->s.v.origin, DRONE_DAMAGE_RADIUS))) {
		if (e == d || e->s.v.takedamage != DAMAGE_AIM || e->s.v.health <= 0) continue;
		roll = g_random();
		base = 40 + roll * 5;
		VectorSubtract(e->s.v.origin, d->s.v.origin, delta); delta[2] += 16;
		distance = vlen(delta);
		if (distance < DRONE_DAMAGE_RADIUS - 1.2f * base) damage = base;
		else if (distance < DRONE_DAMAGE_RADIUS) damage = (DRONE_DAMAGE_RADIUS - distance) * 0.83f;
		else damage = 0;
		if (damage <= 0) continue;
		if (streq(e->classname, "drone")) {
			if (e->cfn.drone_owner == d->cfn.drone_owner) damage *= 0.4f;
			e->s.v.health -= damage;
			if (e->s.v.health <= 0) {
				e->think = (func_t)DroneDie;
				e->s.v.nextthink = g_globalvars.time + 0.01f;
			}
		} else {
			CFN_Damage(e, d, d->cfn.drone_owner, damage, CFN_WEAPON_DRONE);
			if (roll > 0.95f && damage > 20) CFN_BurnSetOnFire(e, d->cfn.drone_owner);
		}
	}
	WriteByte(MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte(MSG_MULTICAST, TE_EXPLOSION);
	WriteCoord(MSG_MULTICAST, d->s.v.origin[0]);
	WriteCoord(MSG_MULTICAST, d->s.v.origin[1]);
	WriteCoord(MSG_MULTICAST, d->s.v.origin[2]);
	trap_multicast(PASSVEC3(d->s.v.origin), MULTICAST_PHS);
	d->classname = "drone_explosion";
	d->s.v.movetype = MOVETYPE_NONE;
	VectorClear(d->s.v.velocity);
	d->touch = (func_t)SUB_Null;
	d->s.v.takedamage = DAMAGE_NO;
	d->s.v.solid = SOLID_NOT;
	setmodel(d, "progs/s_explod.spr");
	d->s.v.frame = 0;
	d->think = (func_t)DroneExplosionFrame;
	d->s.v.nextthink = g_globalvars.time + 0.1f;
}

static void DroneDie(void)
{
	gedict_t *saved_other = other;
	other = self;
	self->cfn.drone_count = -1;
	DroneTouch();
	other = saved_other;
}

static int DroneUpdateTargetData(gedict_t *target, gedict_t *d)
{
	static const float offset[5][2] = {{0,0},{20,20},{-20,-20},{-20,20},{20,-20}};
	vec3_t v;
	int i;
	d->cfn.drone_set_target_vector_time = g_globalvars.time;
	for (i = 0; i < 5; ++i) {
		VectorCopy(target->s.v.origin, v);
		v[0] += offset[i][0]; v[1] += offset[i][1]; v[2] += DRONE_TARGET_OFFSET_Z;
		traceline(PASSVEC3(d->s.v.origin), PASSVEC3(v), true, d);
		if (g_globalvars.trace_fraction == 1) break;
	}
	if (i == 5) return 0;
	d->s.v.enemy = EDICT_TO_PROG(target);
	VectorCopy(v, d->cfn.drone_target_vector);
	VectorCopy(target->s.v.velocity, d->cfn.drone_target_velocity);
	return 1;
}

static int DroneFindTarget(gedict_t *d)
{
	gedict_t *e = world;
	vec3_t forward, center, delta;
	float best = -10, cosine;
	VectorCopy(d->s.v.velocity, forward); VectorNormalize(forward);
	VectorMA(d->s.v.origin, 1000, forward, center);
	while ((e = trap_findradius(e, center, 1050))) {
		if (e->s.v.takedamage != DAMAGE_AIM || e->s.v.health <= 0) continue;
		if (e->cfn.drone_owner == d->cfn.drone_owner && streq(e->classname, "drone")) continue;
		if (!((d->cfn.drone_owner->s.v.waterlevel < 3 && e->s.v.waterlevel < 3)
			|| d->cfn.drone_owner->s.v.waterlevel == e->s.v.waterlevel)) continue;
		VectorSubtract(e->s.v.origin, d->s.v.origin, delta); VectorNormalize(delta);
		cosine = DotProduct(delta, forward);
		if (cosine > best && DroneUpdateTargetData(e, d)) best = cosine;
	}
	d->cfn.drone_targeting_time = g_globalvars.time;
	/* The QC compared -1 although its no-target sentinel was -10. */
	return best != -10;
}

static void DroneThink(void)
{
	gedict_t *d = self, *enemy;
	vec3_t x, v;
	float distance, target_speed, speed, wanted, magnitude;
	int have = 0;
	if (g_globalvars.time > d->cfn.drone_sound_time + 0.8f) {
		sound(d, CHAN_WEAPON, "hknight/hit.wav", 1, ATTN_NORM);
		d->cfn.drone_sound_time = g_globalvars.time;
	}
	if (g_globalvars.time > d->s.v.ltime + DRONE_LIFETIME + 0.5f) {
		d->touch = (func_t)SUB_Null; d->think = (func_t)DroneDie;
		d->s.v.nextthink = g_globalvars.time + 0.01f;
		d->cfn.drone_firstthink = 1;
		return;
	}
	if (VectorCompare(d->cfn.drone_origin_old, d->s.v.origin)) {
		d->s.v.velocity[2] = 150;
		if (++d->cfn.drone_stuck >= 4) { d->s.v.ltime = -100; d->cfn.drone_stuck = 0; }
	} else d->cfn.drone_stuck = 0;
	if ((int)d->s.v.flags & FL_ONGROUND) d->s.v.velocity[2] = 150;
	d->s.v.movetype = MOVETYPE_FLY;
	d->think = (func_t)DroneThink;
	d->s.v.nextthink = g_globalvars.time + DRONE_THINK_NEXTTIME;
	VectorCopy(d->s.v.origin, d->cfn.drone_origin_old);
	d->s.v.flags = (int)d->s.v.flags & ~FL_ONGROUND;
	if (d->cfn.drone_bounced && g_globalvars.time >= d->cfn.drone_bounce_time + 0.2f) {
		setsize(d, -8, -8, -8, 8, 8, 8); d->cfn.drone_bounced = 0;
	}
	DroneThreat(DroneEnemy(d), false);
	if (!d->cfn.drone_firstthink) {
		setsize(d, -8, -8, -8, 8, 8, 8);
		d->cfn.drone_firstthink = 1; d->s.v.owner = EDICT_TO_PROG(d);
		DroneFindTarget(d);
	}
	enemy = DroneEnemy(d);
	VectorSubtract(d->cfn.drone_target_vector, d->s.v.origin, x);
	if (d->cfn.drone_target_is_fixed) have = 3;
	else if (enemy->s.v.deadflag > DEAD_NO) { d->cfn.drone_target_is_fixed = 1; have = 3; }
	else if (g_globalvars.time > d->cfn.drone_targeting_time + 2 && vlen(x) > 150) have = 1;
	if (!have) {
		if (g_globalvars.time > d->cfn.drone_targeting_time + 1 && enemy == d->cfn.drone_owner) have = 1;
		if (!have && g_globalvars.time > d->cfn.drone_set_target_vector_time + 0.5f) have = 2;
	}
	if (have == 1) have = DroneFindTarget(d);
	else if (have == 2) have = DroneUpdateTargetData(enemy, d);
	if (!have) {
		if (vlen(x) < 40) have = 2;
		else {
			VectorCopy(enemy->s.v.origin, v); v[2] += DRONE_TARGET_OFFSET_Z;
			traceline(PASSVEC3(d->s.v.origin), PASSVEC3(v), true, d);
			if (g_globalvars.trace_fraction == 1) have = 2;
		}
		if (have) {
			VectorCopy(enemy->s.v.origin, d->cfn.drone_target_vector);
			d->cfn.drone_target_vector[2] += DRONE_TARGET_OFFSET_Z;
			VectorCopy(enemy->s.v.velocity, d->cfn.drone_target_velocity);
		}
	}
	DroneThreat(DroneEnemy(d), true);
	VectorSubtract(d->cfn.drone_target_vector, d->s.v.origin, x);
	distance = VectorNormalize(x);
	target_speed = vlen(d->cfn.drone_target_velocity);
	VectorCopy(d->s.v.velocity, v); speed = VectorNormalize(v);
	if (DotProduct(v, x) < -0.92f) {
		VectorSubtract(d->cfn.drone_target_vector, d->s.v.origin, x);
		x[0] += 40 * (0.5f - g_random()); x[1] += 40 * (0.5f - g_random());
		x[2] += 30 * g_random(); distance = VectorNormalize(x);
	}
	if (distance < 250) wanted = target_speed + 70 + (1 + DotProduct(v, x)) * distance / 500 * 50;
	else if (distance < 500) wanted = 400;
	else wanted = 600;
	if (speed > wanted) { speed -= 100; if (speed < 150) speed = 150; }
	else if (speed < wanted) { speed += 100; if (speed > wanted) speed = wanted; }
	else if (speed < 150) speed = 150;
	VectorMA(d->s.v.velocity, speed, x, d->s.v.velocity);
	magnitude = vlen(d->s.v.velocity);
	if (!magnitude) VectorMA(d->s.v.velocity, speed + 10, x, d->s.v.velocity);
	else if (magnitude > speed) VectorScale(d->s.v.velocity, speed / magnitude, d->s.v.velocity);
	else if (magnitude < 150) VectorScale(d->s.v.velocity, 150 / magnitude, d->s.v.velocity);
}
