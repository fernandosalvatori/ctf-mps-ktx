/* Runs the actual native port with deterministic engine doubles. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../../source/ktx/src/ctfnormal_shrapnel.c"

gedict_t g_edicts[128], *world = g_edicts, *self, *other;
globalvars_t g_globalvars;
static int used, removed[128], contents, sound_count, blood_count, trace_count;
static int damage_count, burn_count, byte_count, models, sounds, checks;
static float damages[128], random_value, random_queue[64];
static int random_count, random_pos, burn_targets[128], blocked;
static gedict_t *last_attacker, *last_inflictor;
static int last_kind;
static vec3_t plane;

#define CHECK(x) do { ++checks; assert(x); } while (0)
#define NEAR(a,b) CHECK(fabs((a)-(b)) < 0.002f)

void VectorMA(vec3_t a, float s, vec3_t b, vec3_t c) { for(int i=0;i<3;i++) c[i]=a[i]+s*b[i]; }
void VectorScale(vec3_t a, float s, vec3_t b) { for(int i=0;i<3;i++) b[i]=a[i]*s; }
float vlen(vec3_t v) { return sqrtf(DotProduct(v,v)); }
void normalize(vec3_t v, vec3_t o) { float n=vlen(v); for(int i=0;i<3;i++) o[i]=n?v[i]/n:0; }
void vectoangles(vec3_t v, vec3_t o) {
 float yaw=0,pitch;
 if(v[0]==0 && v[1]==0) pitch=v[2]>0?90:270;
 else { yaw=atan2f(v[1],v[0])*180/M_PI; if(yaw<0)yaw+=360; pitch=atan2f(v[2],sqrtf(v[0]*v[0]+v[1]*v[1]))*180/M_PI; if(pitch<0)pitch+=360; }
 o[0]=pitch; o[1]=yaw; o[2]=0;
}
void trap_makevectors(float *a) {
 float p=a[0]*M_PI/180,y=a[1]*M_PI/180,r=a[2]*M_PI/180;
 float sp=sinf(p),cp=cosf(p),sy=sinf(y),cy=cosf(y),sr=sinf(r),cr=cosf(r);
 SetVector(g_globalvars.v_forward,cp*cy,cp*sy,-sp);
 SetVector(g_globalvars.v_right,-sr*sp*cy+cr*sy,-sr*sp*sy-cr*cy,-sr*cp);
 SetVector(g_globalvars.v_up,cr*sp*cy+sr*sy,cr*sp*sy-sr*cy,cr*cp);
}
void aim(vec3_t ret) { VectorCopy(g_globalvars.v_forward,ret); }
float g_random(void) { return random_pos<random_count?random_queue[random_pos++]:random_value; }
gedict_t *spawn(void) { assert(used<128); return &g_edicts[used++]; }
int NUM_FOR_EDICT(gedict_t *p) { return (int)(p-g_edicts); }
void ent_remove(gedict_t *p) { removed[p-g_edicts]=1; }
void SUB_Remove(void) { ent_remove(self); }
void SUB_Null(void) {}
void setmodel(gedict_t *p,char *m) { p->model=m; }
void setsize(gedict_t *p,float a,float b,float c,float d,float e,float f) { SetVector(p->s.v.mins,a,b,c); SetVector(p->s.v.maxs,d,e,f); }
void setorigin(gedict_t *p,float x,float y,float z) { SetVector(p->s.v.origin,x,y,z); }
void sound(gedict_t *p,int ch,char *sample,float vol,float atten) { sound_count++; }
void trap_precache_model(char *s) { models++; }
void trap_precache_sound(char *s) { sounds++; }
void WriteByte(int dest,int value) { byte_count++; CHECK(dest==MSG_ONE); CHECK(value==SVC_SMALLKICK); }
intptr_t trap_pointcontents(float x,float y,float z) { return contents; }
void traceline(float a,float b,float c,float d,float e,float f,int n,gedict_t *p) { trace_count++; VectorCopy(plane,g_globalvars.trace_plane_normal); }
int streq(const char *a,const char *b) { return !strcmp(a?a:"",b?b:""); }
qbool CanDamage(gedict_t *p,gedict_t *inf) { return p-g_edicts!=blocked; }
gedict_t *trap_findradius(gedict_t *start,float *org,float rad) {
 for(int i=(int)(start-g_edicts)+1;i<used;i++) {
  vec3_t d; gedict_t *p=&g_edicts[i]; if(removed[i]||p->s.v.solid==SOLID_NOT)continue;
  for(int a=0;a<3;a++) d[a]=p->s.v.origin[a]+(p->s.v.mins[a]+p->s.v.maxs[a])*0.5f-org[a];
  if(vlen(d)<=rad)return p;
 }
 return NULL;
}
void CFN_Damage(gedict_t *p,gedict_t *inf,gedict_t *att,float d,int kind) {
 CHECK(d>0); CHECK(kind==CFN_WEAPON_SHRAPNEL); damages[p-g_edicts]+=d; damage_count++;
 last_attacker=att;last_inflictor=inf;last_kind=kind;
}
void CFN_BurnSetOnFire(gedict_t *p,gedict_t *att) { burn_count++; burn_targets[p-g_edicts]++;last_attacker=att; }
void spawn_touchblood(float d) { CHECK(d==9);blood_count++; }

static void reset(void) {
 memset(g_edicts,0,sizeof(g_edicts));memset(&g_globalvars,0,sizeof(g_globalvars));
 memset(removed,0,sizeof(removed));memset(damages,0,sizeof(damages));memset(burn_targets,0,sizeof(burn_targets));
 used=2; self=&g_edicts[1];other=world;world->s.v.solid=SOLID_BSP;self->s.v.ammo_rockets=10;self->classname="player";self->connect_time=5;
 contents=CONTENT_EMPTY;sound_count=blood_count=trace_count=damage_count=burn_count=byte_count=models=sounds=0;
 random_count=random_pos=0;random_value=.5f;blocked=-1;SetVector(plane,1,0,0);g_globalvars.time=10;
}
static gedict_t *target(float x,float y,float z,int takedamage) {
 gedict_t *p=spawn();setorigin(p,x,y,z);p->s.v.solid=SOLID_BBOX;p->s.v.takedamage=takedamage;return p;
}
static gedict_t *debris(void) {
 vec3_t d={1,0,0};self->cfn.shrapnel_owner=self;return CFN_ShrapnelDebrisFire(d);
}
static void tick(void) { g_globalvars.time=self->s.v.nextthink;((void(*)(void))self->think)(); }

static void launch_and_sky(void) {
 reset();CFN_ShrapnelPrecache();CHECK(models==3&&sounds==2);CFN_ShrapnelFire();
 gedict_t *p=&g_edicts[2],*f=&g_edicts[3];CHECK(used==4);NEAR(self->s.v.ammo_rockets,9);NEAR(self->s.v.currentammo,9);
 CHECK(byte_count==1&&sound_count==2);CHECK(streq(p->classname,"shrapnel_projectile"));CHECK(p->cfn.shrapnel_owner==self);
 CHECK(PROG_TO_EDICT(p->s.v.owner)==self);NEAR(vlen(p->s.v.velocity),850);NEAR(p->s.v.origin[0],36);NEAR(p->s.v.origin[2],14);
 NEAR(p->s.v.ltime,16);NEAR(p->s.v.nextthink,16);CHECK(p->cfn.weapon_mode==CFN_WEAPON_SHRAPNEL);CHECK(streq(p->model,"progs/grenade.mdl"));
 NEAR(f->s.v.origin[0],18);NEAR(f->s.v.origin[2],14);NEAR(vlen(f->s.v.velocity),850);NEAR(f->s.v.frame,1);CHECK(f->touch==(func_t)SUB_Remove);
 self=p;contents=CONTENT_SKY;CFN_ShrapnelMissileTouch();CHECK(removed[2]&&damage_count==0&&used==4);
 reset();self=debris();contents=CONTENT_SKY;CFN_ShrapnelDebrisTouch();CHECK(removed[self-g_edicts]);CHECK(damage_count==0);
}
static void missile_impacts(void) {
 for(int solid=0;solid<2;solid++)for(int chance=0;chance<2;chance++) {
  reset();random_value=chance?.75f:.25f;CFN_ShrapnelFire();self=&g_edicts[2];other=solid?world:&g_edicts[1];
  CFN_ShrapnelMissileTouch();CHECK(used==4+(solid&&chance?4:3));CHECK(burn_count==(solid&&chance?1:0));
  CHECK(streq(self->model,"progs/s_explod.spr"));CHECK(self->s.v.solid==SOLID_NOT);NEAR(self->s.v.origin[0],32);NEAR(self->s.v.frame,0);
  for(int i=4;i<used;i++){NEAR(vlen(g_edicts[i].s.v.velocity),600);CHECK(g_edicts[i].cfn.shrapnel_owner==&g_edicts[1]);CHECK(g_edicts[i].s.v.owner==EDICT_TO_PROG(world));NEAR(g_edicts[i].s.v.nextthink,g_edicts[i].s.v.ltime+2);}
  for(int frame=1;frame<=5;frame++){tick();NEAR(self->s.v.frame,frame);}tick();CHECK(removed[2]);
 }
 reset();gedict_t *att=self,*enemy=target(0,0,0,DAMAGE_AIM),*shambler=target(0,0,0,DAMAGE_AIM),*shield=target(0,0,0,DAMAGE_AIM);
 shambler->classname="monster_shambler";att->s.v.solid=SOLID_BBOX;att->s.v.takedamage=DAMAGE_AIM;blocked=(int)(shield-g_edicts);
 self=debris();self->s.v.owner=EDICT_TO_PROG(att);CFN_ShrapnelMissileRadius(35);NEAR(damages[att-g_edicts],17.5);NEAR(damages[enemy-g_edicts],35);NEAR(damages[shambler-g_edicts],17.5);NEAR(damages[shield-g_edicts],0);
}
static void fragment_damage(void) {
 reset();gedict_t *att=self,*near=target(0,0,-16,DAMAGE_AIM),*mid=target(35,0,-16,DAMAGE_AIM),*edge=target(64,0,-16,DAMAGE_AIM),*outside=target(0,0,60,DAMAGE_AIM),*object=target(0,0,0,DAMAGE_YES);
 self=debris();other=near;random_value=.8f;blocked=(int)(near-g_edicts);CFN_ShrapnelDebrisExplode(true);
 NEAR(damages[near-g_edicts],23.5f);NEAR(damages[mid-g_edicts],17.5f);NEAR(damages[edge-g_edicts],3);NEAR(damages[outside-g_edicts],0);NEAR(damages[object-g_edicts],10);
 CHECK(burn_targets[near-g_edicts]==1&&burn_targets[mid-g_edicts]==1&&burn_targets[edge-g_edicts]==0);CHECK(last_attacker==att);CHECK(blood_count==1);CHECK(self->s.v.owner==EDICT_TO_PROG(world));
 CHECK(self->s.v.solid==SOLID_NOT);CHECK(self->s.v.movetype==MOVETYPE_NONE);NEAR(vlen(self->s.v.velocity),0);NEAR(self->s.v.frame,0);tick();NEAR(self->s.v.frame,3);tick();NEAR(self->s.v.frame,4);tick();CHECK(removed[self-g_edicts]);
 reset();target(0,0,-16,DAMAGE_AIM);self=debris();CFN_ShrapnelDebrisExplode(false);CHECK(damage_count==0&&burn_count==0);
}
static void touch_and_think(void) {
 reset();gedict_t *victim=target(0,0,-16,DAMAGE_AIM);self=debris();gedict_t *d=self;int initial=used;
 CFN_ShrapnelDebrisTouch();CHECK(self==d);CHECK(used==initial+1);CHECK(streq(d->classname,"shrapnel_debris"));CHECK(damage_count==1&&sound_count==1);CHECK(d->s.v.movetype==MOVETYPE_BOUNCE);
 NEAR(d->cfn.shrapnel_bounce_time,0);NEAR(d->cfn.shrapnel_damage_time,10);NEAR(d->cfn.shrapnel_sound_time,10);
 g_globalvars.time+=.05;CFN_ShrapnelDebrisTouch();CHECK(damage_count==1&&sound_count==1);CHECK(used==initial+2);
 d->s.v.velocity[0]=150;d->s.v.flags=FL_ONGROUND;CFN_ShrapnelDebrisThink();NEAR(vlen(d->s.v.velocity),600);CHECK(d->s.v.movetype==MOVETYPE_FLYMISSILE);CHECK(!((int)d->s.v.flags&FL_ONGROUND));NEAR(d->s.v.nextthink,d->s.v.ltime+2);
 other=victim;g_globalvars.time=d->s.v.ltime+2;CFN_ShrapnelDebrisThink();CHECK(other==victim);CHECK(streq(d->classname,"shrapnel_explosion"));CHECK(damage_count==2);
 reset();self=debris();self->s.v.velocity[0]=100;CFN_ShrapnelDebrisThink();CHECK(streq(self->classname,"shrapnel_explosion"));
 reset();self=debris();self->s.v.ltime=g_globalvars.time;initial=used;CFN_ShrapnelDebrisTouch();CHECK(used==initial);CHECK(streq(self->classname,"shrapnel_explosion"));
 reset();other=target(0,0,0,DAMAGE_AIM);self=debris();initial=used;CFN_ShrapnelDebrisTouch();CHECK(used==initial);CHECK(streq(self->classname,"shrapnel_explosion"));CHECK(blood_count==1);
}
static void owner_lifetime(void) {
 for(int state=0;state<3;state++) {
  reset();gedict_t *att=self;CFN_ShrapnelFire();gedict_t *projectile=&g_edicts[2];
  NEAR(projectile->connect_time,5);CHECK(CFN_ShrapnelOwner(projectile)==att);
  if(state==0)att->cfn.generation++; /* ordinary respawn must retain credit */
  else if(state==1)att->connect_time=10; /* replacement client */
  else att->classname=""; /* disconnected slot, not yet reused */
  gedict_t *expected=state==0?att:world;CHECK(CFN_ShrapnelOwner(projectile)==expected);
  self=projectile;random_value=.8;CFN_ShrapnelMissileTouch();CHECK(last_attacker==expected);
  gedict_t *fragment=&g_edicts[4];NEAR(fragment->connect_time,5);CHECK(CFN_ShrapnelOwner(fragment)==expected);
  target(fragment->s.v.origin[0],fragment->s.v.origin[1],fragment->s.v.origin[2]-16,DAMAGE_AIM);
  self=fragment;CFN_ShrapnelDebrisTouch();CHECK(last_attacker==expected);CHECK(self==fragment);
  gedict_t *proxy=&g_edicts[used-1];NEAR(proxy->connect_time,5);CHECK(CFN_ShrapnelOwner(proxy)==expected);
 }
 reset();gedict_t *att=self;self=debris();target(0,0,-16,DAMAGE_AIM);att->connect_time=11;random_value=.8;CFN_ShrapnelDebrisExplode(true);CHECK(last_attacker==world);
 reset();att=self;CFN_ShrapnelFire();self=&g_edicts[2];target(36,0,14,DAMAGE_AIM);att->connect_time=11;CFN_ShrapnelMissileRadius(35);CHECK(damage_count==1);CHECK(last_attacker==world);
 reset();self=debris();self->cfn.shrapnel_owner=NULL;CHECK(CFN_ShrapnelOwner(self)==world);
}
int main(void) { launch_and_sky();missile_impacts();fragment_damage();touch_and_think();owner_lifetime();printf("PASS: %d assertions; actual Shrapnel source, launch/sky/impact/fragments/radius/burn/bounce/throttle/lifetime/frames/owner-reuse.\n",checks);return 0; }
