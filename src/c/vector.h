#pragma once
#include <pebble.h>

/*
 * Shared GPathInfo arrays for all vector art in the app + one-time scaling.
 *
 * CRITICAL SDK FACT: gpath_create() copies the POINTS POINTER, not the
 * array. Mutating path->points mutates the shared array. So the arrays are
 * scaled exactly once per app lifetime inside vector_create(); callers must
 * use vector_create() and must NEVER touch path->points.
 *
 * Rule for every module: GPath *p = vector_create(&SomePathInfo);
 */

// health.c (foot, heel, zzz, heart)
extern GPathInfo HealthFootPathInfo;
extern GPathInfo HealthHeelPathInfo;
extern GPathInfo HealthZee1PathInfo;
extern GPathInfo HealthZee2PathInfo;
extern GPathInfo HealthZee3PathInfo;
extern GPathInfo HealthHeartPathInfo;

// bluetooth.c
extern GPathInfo BluetoothPathInfo;

// decorations.c (arrows + WR box)
extern GPathInfo ArrowLeftPathInfo;
extern GPathInfo ArrowRightPathInfo;
extern GPathInfo WaterResistOuterPathInfo;

// battery.c (charging bolt)
extern GPathInfo BatteryBoltPathInfo;

// Emery uniform scale factor (1.389 << 10). Identity on other platforms.
#if defined(PBL_PLATFORM_EMERY)
#define GPATH_SCALE_NUM 1422
#else
#define GPATH_SCALE_NUM 1024
#endif

// Create a GPath from a shared GPathInfo with the platform scale applied
// exactly once per array (app lifetime). The only sanctioned constructor
// for the arrays in vector.c.
GPath *vector_create(const GPathInfo *info);
