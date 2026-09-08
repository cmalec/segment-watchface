#pragma once
#include <pebble.h>

/*
 * Shared native Emery GPathInfo arrays for all vector art in the app.
 *
 * GPath creation does not mutate these arrays. Callers use vector_create()
 * so ownership and construction stay consistent across the face.
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

// Create a GPath from a native Emery path definition.
GPath *vector_create(const GPathInfo *info);
