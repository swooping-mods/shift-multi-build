/* Local source copy: each mod is independently loadable. */
#include "api.h"
#include "../../runtime/native/input.h"
#include "../build-menu-framework/sdk/construction_unity.h"
#include "shop_signal.h"
static struct {
    PTR log,world_field,alive,restoring,focused,typing,blocked,first,frame,delta;
    PTR joined,host,local,player,wallet,decimal_number,decimal_int,charge;
    PTR camera,transform,position,forward;
    void (*set_field)(PTR,PTR,void *);
    int (*value_type)(PTR);
    short (*key)(int);PTR (*foreground)(void);DWORD (*pid)(PTR,DWORD *);
    BOOL (*title)(PTR,const char *);
    DRoot world,walker;int last_frame,eligible,host_ready,disabled;DWORD thread;
    unsigned keys[4];
} bm;
static void bevent(const char *s){char b[512],*p=cat(b,"{\"event\":\"");p=cat(p,s);p=cat(p,"\"}\n");DWORD n=0;WriteFile(bm.log,b,(DWORD)(p-b),&n,0);FlushFileBuffers(bm.log);}
static PTR bfield(PTR p,const char *name){return moon_build_field(p,name);}
static int bint(PTR p,const char *name){return moon_build_int(p,name);}
static int bhost(void){int ok=1;if(!boolean(bm.joined,0,&ok))return ok;PTR h=call(bm.host,0,0,&ok),l=call(bm.local,0,0,&ok);U64 a=0,b=0;if(h)dcopy(&a,unbox(h),8);if(l)dcopy(&b,unbox(l),8);return ok&&a&&a==b;}
static int baction(const char *name,int key){return moon_input_key_action(name,(unsigned)key,1).pressed;}
static int bcharge(int cost){int ok=1;PTR id=call(bm.player,0,0,&ok);void *a[]={id};DRoot money=dhold(call(bm.wallet,dget(bm.world),a,&ok));if(!money)return 0;void *c[]={unbox(dget(money))};PTR amount=call(bm.decimal_number,0,c,&ok);int enough=amount&&*(int *)unbox(amount)>=cost;ddrop(&money);if(!ok||!enough)return 0;int minus=-cost;unsigned char value[16]={0};void *ctor[]={&minus};call(bm.decimal_int,value,ctor,&ok);void *debit[]={id,value};if(ok)call(bm.charge,dget(bm.world),debit,&ok);return ok;}
static int bframe(Context *context){
    if(bm.disabled)return 0;if(!bm.thread)bm.thread=GetCurrentThreadId();if(bm.thread!=GetCurrentThreadId())return 0;
    int ok=1,frame=integer(bm.frame,0,&ok);if(!ok||frame==bm.last_frame)return 0;bm.last_frame=frame;
    PTR walker=(PTR)context->rcx,world=0;field_value(walker,bm.world_field,&world);
    if(world!=dget(bm.world)||walker!=dget(bm.walker)){ddrop(&bm.world);ddrop(&bm.walker);bm.world=dhold(world);bm.walker=dhold(walker);bm.eligible=0;return -1;}
    DWORD pid=0;PTR window=bm.foreground();if(window)bm.pid(window,&pid);
    bm.host_ready=world&&!boolean(bm.restoring,world,&ok)&&bhost();
    bm.eligible=bm.host_ready&&pid==GetCurrentProcessId()&&boolean(bm.focused,0,&ok)&&!boolean(bm.typing,0,&ok)&&!boolean(bm.blocked,walker,&ok)&&boolean(bm.first,walker,&ok)&&ok;
    return 1;
}
static void bstatus(const char *s){DWORD pid=0;PTR w=bm.foreground();if(w)bm.pid(w,&pid);if(pid==GetCurrentProcessId())bm.title(w,s);}
static int baim(DVec *origin,DVec *direction){int ok=1;PTR camera=call(bm.camera,0,0,&ok),tr=camera?call(bm.transform,camera,0,&ok):0;if(!tr)return 0;*origin=dvector(bm.position,tr,&ok);*direction=dvector(bm.forward,tr,&ok);return ok;}
static float bscore(DVec origin,DVec direction,DVec point){return moon_build_target_score((MoonBuildPoint){origin.x,origin.y,origin.z},(MoonBuildPoint){direction.x,direction.y,direction.z},(MoonBuildPoint){point.x,point.y,point.z},8,.8f,.005f,1e8f);}
static DRoot benum(PTR collection,int *ok){return moon_build_enumerate(collection,ok);}
static PTR bnext(DRoot iterator,int *ok){return moon_build_next(iterator,bm.value_type,ok);}
static void bend(DRoot *it){moon_build_end(it,bm.value_type);}
static int bstart(PTR domain,PTR game,const char *id){
    if(!bind_runtime(domain,game)||!dapi(game))return 0;
    char folder[1000],path[1200];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,sizeof(folder));if(!n||n>=sizeof(folder))return 0;char *p=cat(path,folder);p=cat(p,"\\");p=cat(p,id);cat(p,".jsonl");bm.log=CreateFileA(path,0x40000000,1,0,1,0x80,0);if(bm.log==(PTR)(U64)-1)return 0;
#define BM(F,NS,T,N,S) bm.F=dmethod(NS,T,N,S);if(!bm.F){bevent(T "::" N);return 0;}
    BM(restoring,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");BM(focused,"UnityEngine","Application","get_isFocused","");BM(typing,"MoonCleaners","MoonInput","get_Typing","");BM(blocked,"MoonCleaners","MoonWalker","get_MenuBlocked","");BM(first,"MoonCleaners","MoonWalker","get_IsFirstPerson","");BM(frame,"UnityEngine","Time","get_frameCount","");BM(delta,"UnityEngine","Time","get_deltaTime","");
    BM(joined,"MoonCleaners","MoonCrew","get_Joined","");BM(host,"MoonCleaners","MoonCrew","get_HostId","");BM(local,"Steamworks","SteamUser","GetSteamID","");BM(player,"MoonCleaners","MoonCrew","get_LocalPlayerId","");BM(wallet,"MoonCleaners","MoonWorld","WalletDollars","System.String");
    /* Decimal has several return-only overloads: resolve the integer converter below. */
#undef BM
    bm.decimal_number=dmethod("System","Decimal","ToInt32","System.Decimal");
    bm.decimal_int=dmethod("System","Decimal",".ctor","System.Int32");bm.charge=dmethod("MoonCleaners","MoonWorld","ChargeBuild","System.String;System.Decimal");bm.camera=dmethod("UnityEngine","Camera","get_main","");bm.transform=dmethod("UnityEngine","Component","get_transform","");bm.position=dmethod("UnityEngine","Transform","get_position","");bm.forward=dmethod("UnityEngine","Transform","get_forward","");
    bm.value_type=(int(*)(PTR))GetProcAddress(game,"il2cpp_class_is_valuetype");bm.world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");bm.set_field=(void(*)(PTR,PTR,void *))GetProcAddress(game,"il2cpp_field_set_value");PTR user=LoadLibraryA("user32.dll");bm.key=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");bm.foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");bm.pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");bm.title=(BOOL(*)(PTR,const char *))GetProcAddress(user,"SetWindowTextA");bm.last_frame=-1;
    (void)moon_shop_unlisten;(void)baction;(void)dzero;(void)dtype;(void)darray;(void)dnumber;(void)bint;(void)bfield;(void)bcharge;(void)bstatus;(void)baim;(void)bscore;(void)benum;(void)bnext;(void)bend;
    return bm.value_type&&bm.world_field&&bm.set_field&&bm.key&&bm.foreground&&bm.pid&&bm.title&&bm.decimal_int&&bm.decimal_number&&bm.charge&&bm.camera&&bm.transform&&bm.position&&bm.forward&&moon_shop_listen(id);
}
