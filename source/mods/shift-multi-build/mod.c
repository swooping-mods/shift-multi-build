#include "../../runtime/native/gameplay.h"
#include "building.h"
int _fltused=0;
static int before(int id,Context *c){
    if(id==0){int state=bframe(c);if(state<0)bevent("session-reset");(void)moon_shop_poll();return 0;}
    /* MachineBuilt is reached only after a successful, paid vanilla placement.
     * Retain its ghost/selected definition instead of switching to belts/cancel.
     * Escape and secondary-click still execute the original CancelPlacement. */
    if(bm.eligible&&!bm.disabled&&GetCurrentThreadId()==bm.thread&&moon_input_action("shift-multi-build.repeat","Shift",1).held){
        if(!moon_build_is_placing((PTR)c->rcx,dget(bm.walker)))return 0;
        bevent("repeat-placement-retained");return 1;
    }return 0;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(!bstart(domain,game,"shift-multi-build"))return 0;
    if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game)||!prepare_hook(method("MoonCleaners","MoonDepotBuilder","MachineBuilt",1),100663899,1,game))return 0;
    before_hook=before;if(!install_hooks())return 0;bevent("ready");return 1;
}
