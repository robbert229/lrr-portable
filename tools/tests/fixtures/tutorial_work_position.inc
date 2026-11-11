// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <strings.h>
#define LEGORRR_BACKEND_WINAPI 1
#define _WIN32 1
#define LWSC_NAME_CAPACITY 128
#define LWSC_MAX_NODES 4096
static int strcasecmp_lite(const char* a,const char* b){return strcasecmp(a,b);}
struct Scene {float p[3];int readable;};
struct EntitySlot {int type;void* attached_scene_object;void* attached_direction_record_type1;};
struct SceneResourceAnimationRecord {};
struct LwscFrameRef {void* interface_pointer;};
struct LwscNode {char native_name[LWSC_NAME_CAPACITY];};
struct LwscSceneTemplate {unsigned count;unsigned order[2];LwscNode nodes[2];};
struct ConfiguredName {const char* key;const char* name;};
struct LwscAssemblyTemplate {unsigned name_count;ConfiguredName names[2];LwscSceneTemplate* scenes[2];};
struct LwscActivityInstance {LwscFrameRef frames[2];};
struct LwscAssemblyInstance {int valid;unsigned current;LwscAssemblyTemplate* assembly;LwscActivityInstance activities[2];};
struct ActorSceneType {LwscAssemblyTemplate* assembly;};
struct ActorSceneInstance {EntitySlot* entity;ActorSceneType* type;LwscAssemblyInstance* instance;void* root_scene;};
static ActorSceneInstance* bound;
static int backend,scene_root;static void* s_actor_backend=&backend;
static ActorSceneInstance* actor_find(EntitySlot* e){return bound&&bound->entity==e?bound:NULL;}
static int actor_valid(ActorSceneInstance* p){return p&&p->instance&&p->instance->valid;}
static void* scene_root_object_get(){return &scene_root;}
static void* lwsc_winapi_scene_object(void*,const LwscFrameRef* f){return f->interface_pointer;}
static void* scene_resource_cached_slot_b4_get(SceneResourceAnimationRecord* p){return p;}
static int ddiscene_object_position_get(void* object,void*,float* out){Scene* p=(Scene*)object;if(!p->readable)return 0;memcpy(out,p->p,sizeof(p->p));return 1;}
/*EXTRACTED*/
static int failures,cases;
static void expect(const char* name,int result,int wanted,const float* out,float x,float y){
 ++cases;if(result!=wanted||out[0]!=x||out[1]!=y){++failures;printf("FAIL %s result=%d output=%g,%g expected=%d/%g,%g\n",name,result,out[0],out[1],wanted,x,y);}
}
static void query(const char* name,EntitySlot* e,int result,float x,float y){float out[2]={-111,-222};int got=entity_work_object_position_get(e,out);expect(name,got,result,out,x,y);}
int main(){
 Scene root={{10,20,30},1},drill_a={{15,25,35},1},drill_b={{18,29,39},1},other={{91,92,93},1};
 LwscSceneTemplate scene[2];memset(scene,0,sizeof(scene));
 LwscAssemblyTemplate assembly;memset(&assembly,0,sizeof(assembly));assembly.name_count=2;
 assembly.names[0].key="CarryNullName";assembly.names[0].name="Other";
 assembly.names[1].key="DrillNullName";assembly.names[1].name="DrillMarker";
 for(unsigned i=0;i<2;++i){scene[i].count=2;scene[i].order[0]=1;scene[i].order[1]=0;strcpy(scene[i].nodes[0].native_name,"DrillMarker_00");strcpy(scene[i].nodes[1].native_name,"Other_00");assembly.scenes[i]=&scene[i];}
 LwscAssemblyInstance instance;memset(&instance,0,sizeof(instance));instance.valid=1;instance.assembly=&assembly;
 instance.activities[0].frames[0].interface_pointer=&drill_a;instance.activities[1].frames[0].interface_pointer=&drill_b;
 instance.activities[0].frames[1].interface_pointer=&other;instance.activities[1].frames[1].interface_pointer=&other;
 ActorSceneType type={&assembly};EntitySlot e={2,&root,NULL};ActorSceneInstance actor={&e,&type,&instance,&root};bound=&actor;
 query("type2-drill-not-root",&e,1,15,25);
 e.type=3;query("type3-drill-not-root",&e,1,15,25);
 instance.current=1;query("current-activity",&e,1,18,29);
 strcpy(scene[1].nodes[0].native_name,"DrillMarker_01");query("ordinal-zero-required",&e,0,-111,-222);
 strcpy(scene[1].nodes[0].native_name,"DRILLMARKER_00");query("case-insensitive-child",&e,1,18,29);
 assembly.names[1].name=NULL;query("missing-configured-name",&e,0,-111,-222);
 assembly.names[1].name="";query("empty-configured-name",&e,0,-111,-222);
 assembly.names[1].name="Absent";query("missing-authored-node",&e,0,-111,-222);
 assembly.names[1].name="DrillMarker";drill_b.readable=0;query("position-failure",&e,0,-111,-222);drill_b.readable=1;
 bound=NULL;query("unbound-actor",&e,0,-111,-222);bound=&actor;
 instance.valid=0;query("invalid-instance",&e,0,-111,-222);instance.valid=1;
 e.type=8;query("unsupported-type",&e,0,-111,-222);
 e.type=1;e.attached_direction_record_type1=&other;query("type1-existing-owner",&e,1,91,92);
 query("null-entity",NULL,0,-111,-222);
 ++cases;if(entity_work_object_position_get(&e,NULL)!=0)++failures;
 printf("cases=%d failures=%d\n",cases,failures);return failures?1:0;
}
