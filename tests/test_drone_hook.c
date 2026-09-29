/* Execute the actual ported modules against deterministic engine boundaries.
 * This checks gameplay math/state; it is not a network or rendered playtest. */
#include "g_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../source/ktx/src/ctfnormal_drone.c"
#include "../source/ktx/src/ctfnormal_hook.c"

gedict_t g_edicts[256], *world = g_edicts, *self, *other;
globalvars_t g_globalvars;
static int count, alive[256], checks, sky, bottom, rays, blocked, burns, bloods;
static float roll = 0.5f, damages[256];
static vec3_t ray_ends[16];
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#c); exit(1); } } while(0)
#define NEAR(a,b) CHECK(fabsf((a)-(b)) < .002f)
float vlen(vec3_t v) { return sqrtf(DotProduct(v,v)); }
float VectorNormalize(vec3_t v) { float n=vlen(v); if(n) {v[0]/=n;v[1]/=n;v[2]/=n;} return n; }
void VectorScale(vec3_t a,float s,vec3_t b) {int i;for(i=0;i<3;i++)b[i]=a[i]*s;}
void VectorMA(vec3_t a,float s,vec3_t b,vec3_t c) {int i;for(i=0;i<3;i++)c[i]=a[i]+s*b[i];}
int VectorCompare(vec3_t a,vec3_t b) {return a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2];}
int strneq(const char*a,const char*b) {return strcmp(a?a:"",b?b:"")!=0;}
int streq(const char*a,const char*b) {return !strneq(a,b);}
float g_random(void) {return roll;}
gedict_t *spawn(void) {gedict_t *e=&g_edicts[++count];alive[count]=1;e->classname="";return e;}
void ent_remove(gedict_t *e) {alive[e-g_edicts]=0;e->s.v.solid=SOLID_NOT;}
gedict_t *find(gedict_t *start,int field,char *s) {int i;for(i=(int)(start-g_edicts)+1;i<=count;i++)if(alive[i]&&streq(g_edicts[i].classname,s))return &g_edicts[i];return NULL;}
gedict_t *trap_findradius(gedict_t *start,float *origin,float rad) {int i;vec3_t d;for(i=(int)(start-g_edicts)+1;i<=count;i++){if(!alive[i]||g_edicts[i].s.v.solid==SOLID_NOT)continue;VectorSubtract(g_edicts[i].s.v.origin,origin,d);if(vlen(d)<=rad)return &g_edicts[i];}return NULL;}
void setorigin(gedict_t *e,float x,float y,float z) {SetVector(e->s.v.origin,x,y,z);}
void setsize(gedict_t *e,float a,float b,float c,float x,float y,float z) {SetVector(e->s.v.mins,a,b,c);SetVector(e->s.v.maxs,x,y,z);VectorSubtract(e->s.v.maxs,e->s.v.mins,e->s.v.size);}
void setmodel(gedict_t *e,char *model) {e->model=model;}
void sound(gedict_t *e,int chan,char *s,float vol,float att) {}
void trap_precache_sound(char *s) {}
void trap_precache_model(char *s) {}
void trap_makevectors(float *angle) {SetVector(g_globalvars.v_forward,1,0,0);}
void aim(vec3_t out) {VectorCopy(g_globalvars.v_forward,out);}
void vectoangles(vec3_t in,vec3_t out) {VectorClear(out);}
void WriteByte(int to,int data) {}
void WriteCoord(int to,float data) {}
intptr_t trap_multicast(float x,float y,float z,intptr_t to) {return 0;}
intptr_t trap_pointcontents(float x,float y,float z) {return sky?CONTENT_SKY:CONTENT_EMPTY;}
int checkbottom(gedict_t *e) {return bottom;}
void AmmoUsed(gedict_t *p) {}
void SUB_Null(void) {}
qbool CFN_Enabled(void) {return true;}
void SpawnBlood(vec3_t origin,float damage) {bloods++;}
void CFN_BurnSetOnFire(gedict_t *p,gedict_t *a) {burns++;}
void traceline(float x,float y,float z,float a,float b,float c,int ignore,gedict_t *e) {if(rays<16)SetVector(ray_ends[rays],a,b,c);g_globalvars.trace_fraction=rays++<blocked?.5f:1;}
void CFN_Damage(gedict_t *t,gedict_t *inflictor,gedict_t *attacker,float damage,int kind) {gedict_t *saved=self;damages[t-g_edicts]+=damage;t->s.v.health-=damage;if(t->s.v.health<=0&&t->th_die){self=t;t->th_die();self=saved;}}
static void reset(void) {memset(g_edicts,0,sizeof(g_edicts));memset(alive,0,sizeof(alive));memset(damages,0,sizeof(damages));memset(&g_globalvars,0,sizeof(g_globalvars));count=sky=bottom=rays=blocked=burns=bloods=0;roll=.5f;g_globalvars.time=10;world->s.v.solid=SOLID_BSP;self=other=world;}
static gedict_t *player(float x,float y,float z) {gedict_t*p=spawn();p->ct=ctPlayer;p->classname="player";p->s.v.health=100;p->s.v.takedamage=DAMAGE_AIM;p->s.v.solid=SOLID_SLIDEBOX;p->s.v.ammo_rockets=20;setorigin(p,x,y,z);return p;}
static gedict_t *drone(gedict_t*p,float x,float y,float z) {gedict_t*d=spawn();d->classname="drone";d->cfn.drone_owner=p;d->s.v.owner=d->s.v.enemy=EDICT_TO_PROG(p);d->s.v.takedamage=DAMAGE_AIM;d->s.v.health=20;d->s.v.solid=SOLID_BBOX;d->s.v.ltime=10;d->th_die=DroneDie;setorigin(d,x,y,z);return d;}

static void test_fire_and_queue(void) {
	gedict_t *p,*d,*first;int i;
	reset();p=player(0,0,0);self=p;p->s.v.items=IT_KEY1|IT_KEY2;
	CFN_DroneFire();d=p->cfn.drone_1;CHECK(d!=NULL);first=d;
	NEAR(p->s.v.ammo_rockets,19);NEAR(p->s.v.currentammo,19);NEAR(d->s.v.origin[0],12);NEAR(d->s.v.origin[2],16);
	NEAR(vlen(d->s.v.velocity),400);NEAR(d->s.v.health,20);NEAR(d->s.v.nextthink,10.6f);CHECK(d->s.v.takedamage==DAMAGE_AIM);
	CHECK(p->s.v.items==(IT_KEY1|IT_KEY2));CHECK(d->cfn.drone_in_queue==1);CHECK(p->cfn.drone_count==1);
	for(i=0;i<4;i++){g_globalvars.time+=.6f;CFN_DroneFire();}
	CHECK(p->cfn.drone_count==15);CHECK(!first->cfn.drone_in_queue);CHECK(first->think==(func_t)DroneDie);CHECK(p->cfn.drone_1!=first);
	CFN_DroneCleanup(p);CHECK(p->cfn.drone_count==0);CHECK(find(world,(int)offsetof(gedict_t,classname),"drone")==NULL);
}
static void test_targeting(void) {
	gedict_t *p,*d,*target,*offaxis;
	reset();p=player(-5000,0,0);d=drone(p,0,0,0);SetVector(d->s.v.velocity,400,0,0);
	target=player(1000,0,0);offaxis=player(100,100,0);CHECK(DroneFindTarget(d)==1);CHECK(DroneEnemy(d)==target);
	rays=0;blocked=4;CHECK(DroneUpdateTargetData(target,d)==1);CHECK(rays==5);NEAR(d->cfn.drone_target_vector[0],1020);NEAR(d->cfn.drone_target_vector[1],-20);NEAR(d->cfn.drone_target_vector[2],16);
	rays=0;blocked=5;CHECK(DroneUpdateTargetData(target,d)==0);CHECK(rays==5);
	rays=blocked=0;target->s.v.waterlevel=3;CHECK(DroneFindTarget(d)==1);CHECK(DroneEnemy(d)==offaxis);
	offaxis->s.v.waterlevel=3;CHECK(DroneFindTarget(d)==0);
	CFN_DroneCleanup(target);CHECK(DroneEnemy(d)==offaxis);
	CFN_DroneCleanup(offaxis);CHECK(DroneEnemy(d)==world);CHECK(d->cfn.drone_target_is_fixed);
}
static void test_guidance(void) {
	gedict_t *p,*d;int i;
	reset();p=player(-5000,0,0);d=drone(p,0,0,0);self=d;d->cfn.drone_target_is_fixed=1;d->cfn.drone_firstthink=1;
	SetVector(d->cfn.drone_target_vector,1000,0,0);SetVector(d->cfn.drone_origin_old,-1,0,0);SetVector(d->s.v.velocity,400,0,0);
	DroneThink();NEAR(vlen(d->s.v.velocity),500);NEAR(d->s.v.nextthink,10.2f);
	d->s.v.origin[0]+=10;g_globalvars.time+=.2f;DroneThink();NEAR(vlen(d->s.v.velocity),600);
	for(i=0;i<4;i++){g_globalvars.time+=.2f;DroneThink();}CHECK(d->s.v.ltime==-100);
	g_globalvars.time+=.2f;DroneThink();CHECK(d->think==(func_t)DroneDie);
	reset();p=player(-5000,0,0);d=drone(p,0,0,0);self=d;g_globalvars.time=16.51f;DroneThink();CHECK(d->think==(func_t)DroneDie);NEAR(d->s.v.nextthink,16.52f);
	reset();p=player(-5000,0,0);d=drone(p,0,0,0);self=d;SetVector(d->s.v.velocity,400,0,0);DroneThink();CHECK(d->cfn.drone_firstthink);CHECK(PROG_TO_EDICT(d->s.v.owner)==d);NEAR(d->s.v.mins[0],-8);NEAR(d->s.v.maxs[0],8);
}
static void test_collisions(void) {
	gedict_t *p,*d,*peer,*victim;float expect;
	reset();p=player(-5000,0,0);d=drone(p,0,0,0);DroneAddToQueue(p,d);self=d;other=world;bottom=1;DroneTouch();
	NEAR(d->s.v.health,17);CHECK(d->cfn.drone_bounced==1);NEAR(d->s.v.nextthink,10.1f);
	DroneTouch();NEAR(d->s.v.health,17);NEAR(d->s.v.origin[2],8);
	g_globalvars.time+=1;peer=drone(p,50,0,0);other=peer;DroneTouch();NEAR(d->s.v.health,14);
	g_globalvars.time+=1;peer->cfn.drone_owner=world;DroneTouch();NEAR(d->s.v.health,8);
	peer->classname="grenade";DroneTouch();NEAR(d->s.v.health,3);
	sky=1;DroneTouch();CHECK(!alive[d-g_edicts]);CHECK(p->cfn.drone_count==0);
	reset();p=player(-5000,0,0);d=drone(p,0,0,0);victim=player(0,0,0);peer=drone(p,0,0,0);self=d;other=victim;roll=.99f;
	DroneTouch();expect=40+.99f*5;NEAR(damages[victim-g_edicts],expect);CHECK(burns==1);NEAR(peer->s.v.health,20-expect*.4f);
	CHECK(streq(d->classname,"drone_explosion"));CHECK(d->s.v.takedamage==DAMAGE_NO);CHECK(d->s.v.solid==SOLID_NOT);
	for(int i=0;i<6;i++)DroneExplosionFrame();CHECK(!alive[d-g_edicts]);
	reset();p=player(-5000,0,0);d=drone(p,0,0,0);victim=player(40,0,-16);self=d;other=victim;DroneTouch();NEAR(damages[victim-g_edicts],24.9f);CHECK(!burns);
	reset();p=player(-5000,0,0);d=drone(p,0,0,0);peer=drone(p,0,0,0);peer->classname="spike";self=d;other=peer;DroneTouch();NEAR(d->s.v.health,14);
}
static void test_hook(void) {
	gedict_t *p,*h,*link,*victim;int n,allocated;
	reset();p=player(0,0,0);self=p;CFN_HookFire();h=p->cfn.hook_next;CHECK(h!=NULL);NEAR(vlen(h->s.v.velocity),1400);NEAR(h->s.v.origin[0],16);NEAR(h->s.v.origin[2],16);NEAR(h->s.v.nextthink,12);
	for(n=0,link=h->cfn.hook_next;link;link=link->cfn.hook_next)n++;CHECK(n==8);allocated=count;CFN_HookFire();CHECK(count==allocated);
	NEAR(h->cfn.hook_next->cfn.hook_xn,8.0f/9);NEAR(p->s.v.ammo_rockets,20);
	setorigin(h,1000,0,16);self=h;other=world;HookTouch();HookThink();NEAR(p->s.v.velocity[0],300);HookThink();NEAR(p->s.v.velocity[0],600);
	CFN_HookDestroy(p);HookThink();CHECK(p->cfn.hook_next==NULL);CHECK(!alive[h-g_edicts]);for(n=3;n<=allocated;n++)CHECK(!alive[n]);
	reset();p=player(0,0,0);self=p;CFN_HookFire();h=p->cfn.hook_next;setorigin(h,50,0,16);self=h;other=world;HookTouch();HookThink();CHECK(p->cfn.hook_status&HOOK_SWING);NEAR(p->s.v.velocity[0],150);
	SetVector(p->s.v.velocity,0,200,0);HookThink();CHECK(p->s.v.velocity[1]>0);NEAR(p->s.v.velocity[0],62.5f);
	reset();p=player(0,0,0);victim=player(200,0,0);self=p;CFN_HookFire();h=p->cfn.hook_next;self=h;other=victim;HookTouch();NEAR(damages[victim-g_edicts],7);CHECK(bloods==1);g_globalvars.time+=2;HookThink();CHECK(p->cfn.hook_next==NULL);
	reset();p=player(0,0,0);self=p;CFN_HookFire();h=p->cfn.hook_next;p->cfn.generation++;p->s.v.frame=99;self=h;HookRemove();CHECK(!alive[h-g_edicts]);NEAR(p->s.v.frame,99);
	reset();p=player(0,0,0);self=p;CFN_HookFire();h=p->cfn.hook_next;sky=1;self=h;other=world;HookTouch();CHECK(h->think==(func_t)HookLandPlayerThenRemove);HookLandPlayerThenRemove();CHECK(p->cfn.hook_next==NULL);
	reset();p=player(0,0,0);self=p;CFN_HookFire();p->teleport_time=11;CFN_HookFrame();CHECK(!(p->cfn.hook_status&HOOK_ACTIVE));NEAR(p->cfn.hook_next->s.v.nextthink,10.0001f);
}
int main(void) {test_fire_and_queue();test_targeting();test_guidance();test_collisions();test_hook();printf("PASS: %d Drone/Hook behavior assertions\n",checks);return 0;}
