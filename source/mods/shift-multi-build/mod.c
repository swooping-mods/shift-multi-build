#include "../../runtime/native/gameplay.h"
#include "building.h"
int _fltused = 0;

static int before(int id, Context *context) {
    if (!id) {
        if (bframe(context) < 0) bevent("session-reset");
        (void)moon_shop_poll();
        return 0;
    }
    PTR builder = (PTR)context->rcx, walker = dget(bm.walker), world = dget(bm.world);
    if (!bm.eligible || bm.disabled || !bm.thread || GetCurrentThreadId() != bm.thread ||
        !context->rsp || !walker || !world || !moon_build_is_placing(builder, walker))
        return 0;
    PTR current_world = 0;
    field_value(walker, bm.world_field, &current_world);
    if (current_world != world) return 0;
    /* At the BeginConveyor call, MachineBuilt has pushed RDI and allocated
     * 0x20 bytes; CALL adds another 8. Its incoming return is therefore +0x30.
     * For the tail-call CancelPlacement, the original return is already RSP. */
    U64 caller = *(U64 *)context->rsp, outer = 0;
    if (id == MOON_REPEAT_CONVEYOR &&
        moon_repeat_rva((U64)bm.game, caller) == MOON_REPEAT_CONVEYOR_RETURN)
        outer = *(U64 *)(context->rsp + MOON_REPEAT_OUTER_RETURN_OFFSET);
    /* Fresh eligibility invokes managed getters. Keep the receiver alive
     * explicitly across those calls, then resolve it again before writes. */
    DRoot builder_root = dhold(builder);
    if (!builder_root) return 0;
    int eligible = beligible(walker, world);
    MoonRepeatGate gate = {eligible, 1, 1, 1, 1,
        moon_input_action("shift-multi-build.repeat", "Shift", eligible).held};
    int retain = moon_repeat_keep(id, (U64)bm.game, caller, outer, gate);
    builder = dget(builder_root);
    if (retain && builder)
        moon_repeat_invalidate(builder, &bm.tickets, bm.set_field);
    else
        retain = 0;
    ddrop(&builder_root);
    if (!retain) return 0;
    bevent("repeat-placement-retained");
    return 1;
}

__declspec(dllexport) int MoonLab_ModStart(PTR domain, PTR game) {
    if (!bstart(domain, game, "shift-multi-build")) return 0;
    PTR cancel = bpinned("CancelPlacement", "", 100663900, 0x436c20);
    PTR conveyor = bpinned("BeginConveyor", "", 100663838, 0x42d6e0);
    if (!cancel || !conveyor ||
        !bpinned("MachineBuilt", "MoonCleaners.MoonDepotBuildDefinition", 100663899, 0x43e900) ||
        !bpinned("UpdatePlacing", "MoonCleaners.MoonDepotBuilder.BuildInput", 100663889, 0x44ede0) ||
        !bpinned("ShowPlan", "MoonCleaners.MoonDepotBuildDefinition;MoonCleaners.MoonWorld.StructurePlan;System.Boolean;MoonCleaners.MoonDepotBuilder.BuildInput;System.String", 100663915, 0x4479c0) ||
        !moon_repeat_contract_matches((const unsigned char *)game)) {
        bevent("repeat-native-contract-rejected");
        return 0;
    }
    if (!prepare_hook(method("MoonCleaners", "MoonWalker", "Update", 0), 100671733, 0, game) ||
        !prepare_hook(cancel, 100663900, MOON_REPEAT_CANCEL, game) ||
        !prepare_hook(conveyor, 100663838, MOON_REPEAT_CONVEYOR, game))
        return 0;
    before_hook = before;
    if (!install_hooks()) return 0;
    bevent("ready");
    return 1;
}
