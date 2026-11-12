#ifndef MOON_SHIFT_MULTI_BUILD_UNITY_H
#define MOON_SHIFT_MULTI_BUILD_UNITY_H
typedef struct {
    PTR check, plan, last_valid, shown, valid, looks_valid, next_check;
} MoonRepeatFields;

/* A retained ghost must obtain a fresh native check for its next click.
 * The IL2CPP setter takes reference values directly (null here), while value
 * fields take scalar addresses. Keep its reference write barrier and retain
 * the native selected kind, pose, snap, grid and input state. */
static inline void moon_repeat_invalidate(PTR builder, const MoonRepeatFields *fields,
                                         void (*set_field)(PTR, PTR, void *)) {
    unsigned char invalid = 0;
    float next = 0;
    set_field(builder, fields->check, 0);
    set_field(builder, fields->plan, 0);
    set_field(builder, fields->last_valid, 0);
    set_field(builder, fields->shown, 0);
    set_field(builder, fields->valid, &invalid);
    set_field(builder, fields->looks_valid, &invalid);
    set_field(builder, fields->next_check, &next);
}
#endif
