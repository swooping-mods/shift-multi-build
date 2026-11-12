#ifndef MOON_SHIFT_MULTI_BUILD_REPEAT_H
#define MOON_SHIFT_MULTI_BUILD_REPEAT_H

enum { MOON_REPEAT_CANCEL = 1, MOON_REPEAT_CONVEYOR = 2 };
enum {
    MOON_REPEAT_MACHINE_RETURN = 0x4505f1,
    MOON_REPEAT_FOUNDATION_RETURN = 0x448d51,
    MOON_REPEAT_CONVEYOR_RETURN = 0x43e9b8,
    MOON_REPEAT_OUTER_RETURN_OFFSET = 0x30,
    MOON_REPEAT_IMAGE_LIMIT = 31917844
};
typedef struct {
    int eligible, enabled, same_thread, owner, placing, held;
} MoonRepeatGate;

static inline int moon_repeat_paid_return(unsigned long long rva) {
    return rva == MOON_REPEAT_MACHINE_RETURN || rva == MOON_REPEAT_FOUNDATION_RETURN;
}
static inline unsigned long long moon_repeat_rva(unsigned long long base,
                                                unsigned long long address) {
    return address >= base && address - base < MOON_REPEAT_IMAGE_LIMIT ? address - base : 0;
}
/* These are the two inspected callers after native TryBuild returned true.
 * MachineBuilt tail-calls CancelPlacement or calls BeginConveyor in its own
 * frame. Neither an input cancellation nor an arbitrary callback is accepted. */
static inline int moon_repeat_keep(int transition, unsigned long long image_base,
                                   unsigned long long caller, unsigned long long outer_caller,
                                   MoonRepeatGate gate) {
    if (!gate.eligible || !gate.enabled || !gate.same_thread || !gate.owner || !gate.placing ||
        !gate.held || !image_base)
        return 0;
    unsigned long long rva = moon_repeat_rva(image_base, caller);
    if (transition == MOON_REPEAT_CANCEL)
        return moon_repeat_paid_return(rva);
    return transition == MOON_REPEAT_CONVEYOR && rva == MOON_REPEAT_CONVEYOR_RETURN &&
           moon_repeat_paid_return(moon_repeat_rva(image_base, outer_caller));
}
#endif
