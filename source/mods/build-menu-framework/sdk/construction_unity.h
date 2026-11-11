#ifndef MOONLAB_CONSTRUCTION_UNITY_H
#define MOONLAB_CONSTRUCTION_UNITY_H
#include "construction.h"

/* Shared reflection subset must be available before this adapter. Roots own
 * checks/plans; enumeration values are borrowed until the next MoveNext. */
static inline PTR moon_build_field(PTR object, const char *name) {
    PTR value = 0, field = object ? get_field(object_class(object), name) : 0;
    if (field)
        field_value(object, field, &value);
    return value;
}
static inline int moon_build_int(PTR object, const char *name) {
    int value = 0;
    PTR field = object ? get_field(object_class(object), name) : 0;
    if (field)
        field_value(object, field, &value);
    return value;
}
static inline int moon_build_is_placing(PTR builder, PTR walker) {
    if (!builder || moon_build_field(builder, "walker") != walker)
        return 0;
    unsigned char placing = 0;
    PTR field = get_field(object_class(builder), "placing");
    if (field)
        field_value(builder, field, &placing);
    return placing != 0;
}
static inline DRoot moon_build_enumerate(PTR collection, int *ok) {
    return dhold(
        collection ? call(get_method(object_class(collection), "GetEnumerator", 0), collection, 0, ok) : 0);
}
static inline PTR moon_build_iterator_self(DRoot iterator, int (*value_type)(PTR), PTR *klass) {
    PTR boxed = dget(iterator);
    *klass = boxed ? object_class(boxed) : 0;
    return boxed ? (value_type(*klass) ? unbox(boxed) : boxed) : 0;
}
static inline PTR moon_build_next(DRoot iterator, int (*value_type)(PTR), int *ok) {
    PTR klass = 0, self = moon_build_iterator_self(iterator, value_type, &klass);
    if (!self || !boolean(get_method(klass, "MoveNext", 0), self, ok))
        return 0;
    return call(get_method(klass, "get_Current", 0), self, 0, ok);
}
static inline void moon_build_end(DRoot *iterator, int (*value_type)(PTR)) {
    int ok = 1;
    PTR klass = 0, self = moon_build_iterator_self(*iterator, value_type, &klass),
        dispose = klass ? get_method(klass, "Dispose", 0) : 0;
    if (dispose)
        call(dispose, self, 0, &ok);
    ddrop(iterator);
}
typedef struct {
    PTR plan, begin_structure, begin_depot, build_structure, build_depot;
} MoonBuildPlacementApi;
typedef struct {
    DRoot check, plan, world;
    MoonBuildPoint point;
    float yaw;
    int kind, foundation;
    float elapsed;
    int ready, attempted;
} MoonBuildPlacement;
enum {
    MOON_BUILD_WAIT = 0,
    MOON_BUILD_READY = 1,
    MOON_BUILD_UNAVAILABLE = -1,
    MOON_BUILD_TIMEOUT = -2,
    MOON_BUILD_SIGNATURE = -3,
    MOON_BUILD_INVALID = -4
};
static inline int moon_build_placement_bind(MoonBuildPlacementApi *api) {
#define MC(F, M, S)                                                                                          \
    api->F = dmethod("MoonCleaners", "MoonWorld", M, S);                                                     \
    if (!api->F)                                                                                             \
    return 0
    MC(plan, "PlanStructure",
       "System.Int32;UnityEngine.Vector3;System.Single;System.Int32;System.Boolean;System.Single;System."
       "Int32;UnityEngine.Vector3");
    MC(begin_structure, "BeginStructureCheck", "MoonCleaners.MoonWorld.StructurePlan");
    MC(begin_depot, "BeginDepotCheck", "UnityEngine.Vector3;System.Single;System.Int32");
    MC(build_structure, "TryBuildStructure",
       "MoonCleaners.MoonWalker;MoonCleaners.MoonWorld.StructurePlan;MoonCleaners.MoonWorld."
       "DepotPlacementCheck;System.String&");
    MC(build_depot, "TryBuildDepot",
       "MoonCleaners.MoonWalker;UnityEngine.Vector3;System.Single;System.String&;MoonCleaners.MoonWorld."
       "DepotPlacementCheck;System.Int32");
#undef MC
    return 1;
}
static inline void moon_build_placement_cancel(MoonBuildPlacement *placement) {
    ddrop(&placement->check);
    ddrop(&placement->plan);
    ddrop(&placement->world);
    placement->elapsed = 0;
    placement->ready = placement->attempted = 0;
}
static inline int moon_build_placement_begin(MoonBuildPlacement *p, const MoonBuildPlacementApi *api,
                                             PTR world, int kind, int foundation, MoonBuildPoint point,
                                             float yaw) {
    moon_build_placement_cancel(p);
    int ok = 1;
    p->world = dhold(world);
    if (!p->world)
        return 0;
    p->kind = kind;
    p->foundation = foundation;
    p->point = point;
    p->yaw = yaw;
    if (foundation) {
        int turn = 0, support = -1;
        unsigned char free_mode = 0;
        float lift = 0;
        MoonBuildPoint heading = {0};
        void *a[] = {&kind, &point, &yaw, &support, &free_mode, &lift, &turn, &heading};
        p->plan = dhold(call(api->plan, world, a, &ok));
        void *b[] = {dget(p->plan)};
        if (p->plan && ok)
            p->check = dhold(call(api->begin_structure, world, b, &ok));
    } else {
        void *a[] = {&point, &yaw, &kind};
        p->check = dhold(call(api->begin_depot, world, a, &ok));
    }
    if (!p->check || !ok) {
        moon_build_placement_cancel(p);
        return 0;
    }
    return 1;
}
static inline int moon_build_placement_advance(MoonBuildPlacement *p, float dt, float timeout,
                                               double budget_ms) {
    if (!p->check || p->attempted)
        return MOON_BUILD_UNAVAILABLE;
    if (p->ready)
        return MOON_BUILD_READY;
    int ok = 1;
    p->elapsed += dt;
    PTR check = dget(p->check), advance = get_method(object_class(check), "Advance", 1);
    char *signature = advance ? da.type_name(da.param(advance, 0)) : 0;
    int signature_ok = signature && deq(signature, "System.Double");
    if (signature)
        da.free(signature);
    if (!signature_ok)
        return MOON_BUILD_SIGNATURE;
    void *a[] = {&budget_ms};
    call(advance, check, a, &ok);
    if (p->elapsed > timeout)
        return MOON_BUILD_TIMEOUT;
    if (!ok)
        return MOON_BUILD_UNAVAILABLE;
    int done = boolean(get_method(object_class(check), "get_Done", 0), check, &ok);
    if (!ok)
        return MOON_BUILD_UNAVAILABLE;
    if (!done)
        return MOON_BUILD_WAIT;
    if (!boolean(get_method(object_class(check), "get_Valid", 0), check, &ok) || !ok)
        return MOON_BUILD_INVALID;
    p->ready = 1;
    return MOON_BUILD_READY;
}
/* Exactly one native commit attempt per accepted check. Native TryBuild owns
 * unlocks, collision validation and payment; no wallet manipulation lives here. */
static inline int moon_build_placement_commit(MoonBuildPlacement *p, const MoonBuildPlacementApi *api,
                                              PTR world, PTR walker) {
    if (!p->ready || p->attempted || !p->check || (p->foundation && !p->plan) || world != dget(p->world))
        return 0;
    p->attempted = 1;
    int ok = 1;
    PTR reason = 0, result = 0;
    if (p->foundation) {
        void *a[] = {walker, dget(p->plan), dget(p->check), &reason};
        result = call(api->build_structure, world, a, &ok);
    } else {
        void *a[] = {walker, &p->point, &p->yaw, &reason, dget(p->check), &p->kind};
        result = call(api->build_depot, world, a, &ok);
    }
    return ok && result && *(unsigned char *)unbox(result);
}
#endif
