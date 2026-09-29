/* Tests link the actual ported units and mock only the KTX/engine boundary. */
#include "g_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

gedict_t g_edicts[256], *world = g_edicts, *self, *other;
globalvars_t g_globalvars;
static int checks, used, removed[256], contents, enabled, teammates;
static float random_value, damage_values[256];
static int damage_kind[256], test_attacker[256], sounds, models, blood;
static const char *last_sound;

#define CHECK(x) do { ++checks; if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
#define CLOSE(a,b) CHECK(fabs((a)-(b)) < 0.001)
static int indexof(gedict_t *p) { return (int)(p - g_edicts); }
int NUM_FOR_EDICT(gedict_t *p) { return indexof(p); }
int streq(const char *a, const char *b) { return a && b && !strcmp(a,b); }
qbool CFN_Enabled(void) { return enabled; }
qbool isCTF(void) { return true; }
qbool SameTeam(gedict_t *a, gedict_t *b) { return a == b || teammates; }
float g_random(void) { return random_value; }
float crandom(void) { return random_value * 2 - 1; }
float vlen(vec3_t a) { return sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]); }
void normalize(vec3_t a, vec3_t b) { float n=vlen(a); for(int i=0;i<3;i++) b[i]=n?a[i]/n:0; }
void VectorMA(vec3_t a,float s,vec3_t b,vec3_t c) { for(int i=0;i<3;i++) c[i]=a[i]+s*b[i]; }
void VectorScale(vec3_t a,float s,vec3_t b) { for(int i=0;i<3;i++) b[i]=a[i]*s; }
void vectoangles(vec3_t a,vec3_t b) { SetVector(b,0,0,0); }
void trap_makevectors(vec3_t a) { SetVector(g_globalvars.v_forward,1,0,0); SetVector(g_globalvars.v_right,0,-1,0); }
intptr_t trap_pointcontents(float x,float y,float z) { return contents; }
void trap_precache_sound(char *name) { ++sounds; }
void trap_precache_model(char *name) { ++models; }
void setmodel(gedict_t *e,char *name) { e->model=name; }
void setorigin(gedict_t *e,float x,float y,float z) { SetVector(e->s.v.origin,x,y,z); }
void setsize(gedict_t *e,float a,float b,float c,float d,float f,float g) { SetVector(e->s.v.mins,a,b,c); SetVector(e->s.v.maxs,d,f,g); }
void sound(gedict_t *e,int ch,char *s,float vol,float att) { ++sounds; last_sound=s; }
gedict_t *spawn(void) { CHECK(used<256); gedict_t *e=&g_edicts[used++]; memset(e,0,sizeof(*e)); e->classname=""; return e; }
void ent_remove(gedict_t *e) { CHECK(e && e!=world); CHECK(!removed[indexof(e)]); removed[indexof(e)]=1; }
void SUB_Remove(void) { ent_remove(self); }
void SUB_Null(void) {}
void bubble_bob(void) {}
void barrel_explode(void) {}
void spawn_touchblood(float damage) { blood += (int)damage; }
void CFN_Damage(gedict_t *v,gedict_t *inf,gedict_t *a,float d,int kind) { damage_values[indexof(v)]+=d; damage_kind[indexof(v)]=kind; test_attacker[indexof(v)]=indexof(a); }
gedict_t *trap_findradius(gedict_t *start,float *org,float radius) {
 for(int i=indexof(start)+1;i<used;i++) {
  vec3_t d; gedict_t *e=&g_edicts[i];
  if(removed[i] || e->s.v.solid==SOLID_NOT) continue;
  for(int k=0;k<3;k++) d[k]=e->s.v.origin[k]+(e->s.v.mins[k]+e->s.v.maxs[k])*0.5f-org[k];
  if(vlen(d)<=radius) return e;
 }
 return NULL;
}
static void reset(void) {
 memset(g_edicts,0,sizeof(g_edicts)); memset(&g_globalvars,0,sizeof(g_globalvars));
 memset(removed,0,sizeof(removed)); memset(damage_values,0,sizeof(damage_values));
 used=4; self=&g_edicts[1]; other=world; contents=CONTENT_EMPTY; enabled=1; teammates=0;
 sounds=models=blood=0; random_value=0.5; g_globalvars.time=10;
 for(int i=0;i<4;i++) g_edicts[i].classname="player";
 for(int i=1;i<4;i++) { g_edicts[i].s.v.health=100; g_edicts[i].s.v.takedamage=DAMAGE_AIM; }
}
static void think(gedict_t *e) { self=e; ((void (*)(void))e->think)(); }
static gedict_t *ignite(int stacks) {
 gedict_t *v=&g_edicts[2];
 for(int i=0;i<stacks;i++) CFN_BurnSetOnFire(v,&g_edicts[1]);
 return v->cfn.burn_flame;
}
static void test_weld(void) {
 vec3_t org={10,20,30},dir={1,0,0};
 reset(); CFN_WeldPrecache(); CHECK(models==2); CHECK(sounds==3);
 CFN_WeldFire(org,dir); gedict_t *b=&g_edicts[4];
 CHECK(PROG_TO_EDICT(b->s.v.owner)==&g_edicts[1]); CHECK(streq(b->classname,"weld_blob"));
 CLOSE(b->s.v.velocity[0],1400); CLOSE(b->s.v.nextthink,16);
 CLOSE(b->s.v.origin[0],18); CLOSE(b->s.v.origin[1],20); CLOSE(b->s.v.origin[2],24);
 CHECK(b->s.v.movetype==MOVETYPE_FLYMISSILE && b->s.v.solid==SOLID_BBOX);
 CHECK((int)b->s.v.effects & EF_DIMLIGHT); CLOSE(g_edicts[1].cfn.weld_light_time,10.2);
 CFN_WeldFire(org,dir); CHECK(!(int)g_edicts[5].s.v.effects);
 self=b; contents=CONTENT_SKY; ((void(*)(void))b->touch)(); CHECK(removed[4]);

 /* The actual damage function is exercised against the original piecewise formula. */
 for(int ri=0;ri<20;ri++) for(int dist=0;dist<=65;dist+=5) {
  reset(); random_value=ri/20.0f; SetVector(org,0,0,6); CFN_WeldFire(org,dir); b=&g_edicts[4];
  gedict_t *v=&g_edicts[2]; v->s.v.solid=SOLID_SLIDEBOX;
  setorigin(v,b->s.v.origin[0]+dist,0,-16); self=b; other=v;
  ((void(*)(void))b->touch)();
  float db=11+(0.5f-random_value)*6;
  float expected=dist<3*db?db:dist<60?db*(60-dist)/(60-3*db):0;
  /* QC radius selection uses bbox center; zero bbox here is 16 units below impact. */
  if(sqrtf(dist*dist+256)>60) expected=0;
  CLOSE(damage_values[2],expected); CHECK(blood==9);
  CHECK((v->cfn.burn_burning!=0)==(random_value>0.85f && expected>5));
  CHECK(b->s.v.frame==0); CLOSE(b->s.v.origin[0],4);
  g_globalvars.time+=0.1f; think(b); CHECK(b->s.v.frame==3);
  g_globalvars.time+=0.1f; think(b); CHECK(b->s.v.frame==4);
  g_globalvars.time+=0.1f; think(b); CHECK(removed[4]);
 }
 reset(); SetVector(org,0,0,6); CFN_WeldFire(org,dir); b=&g_edicts[4];
 g_edicts[2].s.v.solid=SOLID_BBOX; g_edicts[2].s.v.takedamage=DAMAGE_YES;
 self=b; ((void(*)(void))b->touch)(); CLOSE(damage_values[2],10); CHECK(damage_kind[2]==CFN_WEAPON_WELD);
 /* Flight continues after owner disconnect, without crediting a replacement slot. */
 for(int disconnected=0;disconnected<2;disconnected++) {
  reset(); SetVector(org,0,0,6); g_edicts[1].connect_time=1; CFN_WeldFire(org,dir); b=&g_edicts[4];
  if(disconnected) g_edicts[1].classname=""; else g_edicts[1].connect_time=2;
  g_edicts[2].s.v.solid=SOLID_BBOX; g_edicts[2].s.v.takedamage=DAMAGE_YES;
  self=b; ((void(*)(void))b->touch)(); CLOSE(damage_values[2],10); CHECK(test_attacker[2]==0);
 }
}
static void test_burn(void) {
 reset(); CFN_BurnPrecache(); CHECK(models==3 && sounds==4);
 for(int blocker=0;blocker<7;blocker++) {
  reset(); gedict_t *v=&g_edicts[2];
  if(blocker==0) enabled=0;
  if(blocker==1) v->s.v.waterlevel=2;
  if(blocker==2) v->invincible_finished=10;
  if(blocker==3) v->classname="drone";
  if(blocker==4) v->th_die=barrel_explode;
  if(blocker==5) v->s.v.health=0;
  if(blocker==6) teammates=1;
  CFN_BurnSetOnFire(v,&g_edicts[1]); CHECK(!v->cfn.burn_burning && used==4);
 }
 reset(); teammates=1; CFN_BurnSetOnFire(self,self); CHECK(self->cfn.burn_burning==1);
 for(int n=1;n<=3;n++) {
  reset(); gedict_t *f=ignite(n),*v=&g_edicts[2];
  CHECK((int)v->cfn.burn_burning==(1<<n)-1); CHECK(used==6);
  CHECK(PROG_TO_EDICT(f->s.v.owner)==&g_edicts[1]); CHECK(f->s.v.frame==1);
  CLOSE(f->s.v.nextthink,10.1); CLOSE(f->cfn.burn_lifetime1,25);
  g_globalvars.time=10.1f; think(f); CLOSE(damage_values[2],n*3);
  CLOSE(f->s.v.nextthink,10.12); CLOSE(f->cfn.burn_damage_time,11.1);
  think(f); CLOSE(damage_values[2],n*3); /* 50Hz visual updates do not deal damage each time. */
  CFN_BurnCleanup(v); CHECK(!v->cfn.burn_burning && !v->cfn.burn_flame && !v->cfn.burn_flame2);
  CHECK(removed[4] && removed[5]); CFN_BurnCleanup(v);
 }
 /* Equal timestamps refresh stack1; refresh order follows original nested branches. */
 reset(); gedict_t *f=ignite(3); g_globalvars.time=11; CFN_BurnSetOnFire(&g_edicts[2],self);
 CLOSE(f->cfn.burn_lifetime1,26); CLOSE(f->cfn.burn_lifetime2,25);
 CFN_BurnSetOnFire(&g_edicts[2],self); CLOSE(f->cfn.burn_lifetime2,26);
 CFN_BurnSetOnFire(&g_edicts[2],self); CLOSE(f->cfn.burn_lifetime4,25); /* Original no-op branch. */
 /* Last expired tick still applies its 3 damage, then removes the flames. */
 reset(); f=ignite(1); g_globalvars.time=25.1; think(f);
 CLOSE(damage_values[2],3); CHECK(removed[4] && removed[5]); CHECK(!g_edicts[2].cfn.burn_burning);
 /* Expire each independent stack; strict > comparison retains equality. */
 reset(); f=ignite(3); f->cfn.burn_lifetime1=9; f->cfn.burn_lifetime2=10; f->cfn.burn_lifetime4=11;
 think(f); CLOSE(damage_values[2],9); CHECK(g_edicts[2].cfn.burn_burning==6);
 /* Contagion has radius50, damage6+r*4 and credits the burning player. */
 reset(); f=ignite(1); random_value=0.75; gedict_t *v=&g_edicts[3];
 v->s.v.solid=SOLID_SLIDEBOX; setorigin(v,40,0,18); think(f);
 CLOSE(damage_values[2],3); CLOSE(damage_values[3],9); CHECK(test_attacker[3]==2);
 CHECK(v->cfn.burn_burning==1); CHECK(PROG_TO_EDICT(v->cfn.burn_flame->s.v.owner)==&g_edicts[2]);
 reset(); f=ignite(1); random_value=0.5; g_edicts[3].s.v.solid=SOLID_SLIDEBOX;
 setorigin(&g_edicts[3],40,0,18); think(f); CLOSE(damage_values[3],8); CHECK(!g_edicts[3].cfn.burn_burning);
 /* Water extinguishes on next damage tick and produces eight delayed bubbles. */
 reset(); f=ignite(3); g_edicts[2].s.v.waterlevel=2; think(f);
 CHECK(removed[4] && removed[5]); CLOSE(damage_values[2],0); CHECK(streq(last_sound,"player/slimbrn2.wav"));
 gedict_t *steam=&g_edicts[6]; CHECK(steam->bubble_count==8);
 for(int i=0;i<8;i++) { think(steam); CHECK(streq(g_edicts[7+i].classname,"bubble")); CLOSE(g_edicts[7+i].s.v.velocity[2],15); }
 CHECK(removed[6]);
 /* A death from another weapon removes flames; Burn deaths retain full sprite animation. */
 reset(); f=ignite(1); g_edicts[2].s.v.deadflag=DEAD_DYING; g_edicts[2].cfn.killweapon=CFN_WEAPON_WELD;
 think(f); CHECK(removed[4] && removed[5]);
 reset(); f=ignite(1); g_edicts[2].s.v.deadflag=DEAD_DEAD; g_edicts[2].cfn.killweapon=CFN_WEAPON_BURN;
 think(f); CHECK(!removed[4] && removed[5]); CHECK(streq(f->model,"progs/s_explod.spr"));
 CHECK(f->s.v.frame==0); for(int i=1;i<=5;i++) { think(f); CHECK(f->s.v.frame==i); } think(f); CHECK(removed[4]);
 /* Pain samples and original independent 0.8s rate limit. */
 reset(); self=&g_edicts[2]; random_value=0.6; CFN_BurnPainSound(); CHECK(streq(last_sound,"player/lburn1.wav"));
 int previous=sounds; CFN_BurnPainSound(); CHECK(sounds==previous);
 g_globalvars.time=10.9; random_value=0.4; CFN_BurnPainSound(); CHECK(streq(last_sound,"player/lburn2.wav"));
 /* Disconnect and slot reuse do not transfer credit. A respawn retains credit. */
 for(int mode=0;mode<3;mode++) {
  reset(); g_edicts[1].connect_time=1; f=ignite(1);
  if(mode==0) g_edicts[1].connect_time=2;
  if(mode==1) g_edicts[1].classname="";
  if(mode==2) g_edicts[1].cfn.generation++;
  think(f); CLOSE(damage_values[2],3); CHECK(test_attacker[2]==(mode==2?1:0));
 }
 /* Old steam cannot follow a different player reusing its victim's slot. */
 reset(); f=ignite(1); g_edicts[2].s.v.waterlevel=2; think(f); steam=&g_edicts[6];
 int oldused=used; g_edicts[2].connect_time=20; think(steam); CHECK(removed[6]); CHECK(used==oldused);
}
int main(void) { test_weld(); test_burn(); printf("PASS: %d checks using the actual Weld/Burn C units\n",checks); return 0; }
