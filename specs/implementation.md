# Shift Multi-Build: behavior and implementation

Describes the exported implementation **0.1.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Holding Shift after a successful native machine placement keeps the selected building ghost for another placement. A pinned MachineBuilt transition runs only for an eligible local host. The original placement still performs its own site validation, price/payment and native cancellation; the feature repeats selection rather than issuing extra build commits.

## State and lifecycle

The held repeat action is session input; completed structures use normal saves. World/host/focus/menu/tool gates remain in the shared construction adapter. There is no new blueprint or queue format.

## Integration and configuration

No additional feature-mod runtime dependency is declared. Included shared headers are source reuse, not extra DLL dependencies.

The [module metadata](../source/mods/shift-multi-build/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

## Implementation sources

- [mod.c](../source/mods/shift-multi-build/mod.c)
- [building.h](../source/mods/shift-multi-build/building.h)

## Verification boundary

Foundation-specific repeat interception and other construction modes are not guaranteed by this machine-placement hook. Cost, cancellation, held-key transitions and combined build mods require game qualification.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
