#ifndef MOONLAB_CONSTRUCTION_H
#define MOONLAB_CONSTRUCTION_H

/* Placement geometry has no registration, save-kind allocation or prices.
 * Features supply their original bounds/tolerance and native code owns costs. */
typedef struct {
    float x, y, z;
} MoonBuildPoint;
static inline float moon_build_distance2(MoonBuildPoint a, MoonBuildPoint b) {
    float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return x * x + y * y + z * z;
}
static inline int moon_build_in_box(MoonBuildPoint p, MoonBuildPoint a, MoonBuildPoint b, float padding) {
    float lo = a.x < b.x ? a.x : b.x, hi = a.x > b.x ? a.x : b.x;
    if (!(p.x >= lo - padding && p.x <= hi + padding))
        return 0;
    lo = a.y < b.y ? a.y : b.y;
    hi = a.y > b.y ? a.y : b.y;
    if (!(p.y >= lo - padding && p.y <= hi + padding))
        return 0;
    lo = a.z < b.z ? a.z : b.z;
    hi = a.z > b.z ? a.z : b.z;
    return p.z >= lo - padding && p.z <= hi + padding;
}
/* direction is the native camera's unit forward; preserve the caller's
 * strict cross-section cutoff and depth bias when choosing a target. */
static inline float moon_build_target_score(MoonBuildPoint origin, MoonBuildPoint direction,
                                            MoonBuildPoint point, float reach, float tolerance2,
                                            float depth_bias, float rejected) {
    MoonBuildPoint d = {point.x - origin.x, point.y - origin.y, point.z - origin.z};
    float t = d.x * direction.x + d.y * direction.y + d.z * direction.z;
    if (t < 0 || t > reach)
        return rejected;
    float q = d.x * d.x + d.y * d.y + d.z * d.z - t * t;
    return q < tolerance2 ? q + t * depth_bias : rejected;
}
#endif
