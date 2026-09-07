#include <pebble.h>
#include "vector.h"

/*
 * GPathInfo arrays and one-time scaling.
 *
 * CRITICAL SDK FACT: gpath_create() copies the POINTS POINTER from the
 * GPathInfo, not the array contents. Any mutation through path->points
 * mutates the shared array. Therefore the arrays below are scaled EXACTLY
 * ONCE per app lifetime (guarded by s_scaled), and every GPath created from
 * them afterwards shares the scaled points. Never scale a second time —
 * see the garbled-icons incident.
 */

GPathInfo HealthFootPathInfo = {
  .num_points = 13,
  .points = (GPoint []) {{0,1}, {1,1}, {1,0}, {2,0}, {2,1}, {3,1}, {3,3}, {2,3}, {2,5}, {1,5}, {1,4}, {0,4}, {0,1}}
};
GPathInfo HealthHeelPathInfo = {
  .num_points = 4,
  .points = (GPoint []) {{1,7}, {2,7}, {2,8}, {1,8}}
};
GPathInfo HealthZee1PathInfo = {
  .num_points = 6,
  .points = (GPoint []) {{0,2}, {2,2}, {2,3}, {0,5}, {0,6}, {2,6}}
};
GPathInfo HealthZee2PathInfo = {
  .num_points = 6,
  .points = (GPoint []) {{3,1}, {6,1}, {6,2}, {3,5}, {3,6}, {6,6}}
};
GPathInfo HealthZee3PathInfo = {
  .num_points = 6,
  .points = (GPoint []) {{7,0}, {11,0}, {11,1}, {7,5}, {7,6}, {11,6}}
};
GPathInfo HealthHeartPathInfo = {
  .num_points = 13,
  .points = (GPoint []) {{0,4}, {2,1}, {4,2}, {5,3}, {6,2}, {8,1}, {10,4}, {5,10}, {0,4}, {0,4}, {0,4}, {0,4}, {0,4}}
};

GPathInfo BluetoothPathInfo = {
  .num_points = 18,
  .points = (GPoint []) {{0, 1}, {2, 3}, {3, 3}, {3, 0}, {4, 0}, {6, 2}, {4, 4}, {6, 6}, {4, 8}, {3, 8}, {3, 5}, {2, 5}, {0, 7}, {2, 5}, {3, 5}, {3, 3}, {2, 3}, {0, 1}}
};

GPathInfo ArrowLeftPathInfo = {
  .num_points = 5,
  .points = (GPoint []) {{0,2}, {4,0}, {4,5}, {0,3}, {0,2}}
};
GPathInfo ArrowRightPathInfo = {
  .num_points = 5,
  .points = (GPoint []) {{0,0}, {4,2}, {4,3}, {0,5}, {0,0}}
};
GPathInfo WaterResistOuterPathInfo = {
  .num_points = 9,
  .points = (GPoint []) {{0,2}, {2,0}, {39,0}, {41,2}, {41,9}, {35,15}, {5,15}, {0,10}, {0,2}}
};

static bool s_scaled = false;

static void scale_info_once(GPathInfo *info) {
  for (uint32_t i = 0; i < info->num_points; i++) {
    info->points[i].x = (info->points[i].x * GPATH_SCALE_NUM) >> 10;
    info->points[i].y = (info->points[i].y * GPATH_SCALE_NUM) >> 10;
  }
}

static void ensure_scaled(void) {
  if (s_scaled) {
    return;
  }
#if defined(PBL_PLATFORM_EMERY)
  scale_info_once(&HealthFootPathInfo);
  scale_info_once(&HealthHeelPathInfo);
  scale_info_once(&HealthZee1PathInfo);
  scale_info_once(&HealthZee2PathInfo);
  scale_info_once(&HealthZee3PathInfo);
  scale_info_once(&HealthHeartPathInfo);
  scale_info_once(&BluetoothPathInfo);
  scale_info_once(&ArrowLeftPathInfo);
  scale_info_once(&ArrowRightPathInfo);
  scale_info_once(&WaterResistOuterPathInfo);
#endif
  s_scaled = true;
}

// Create a GPath from a shared GPathInfo, with the platform scale applied
// exactly once per array (app lifetime). This is the ONLY sanctioned way to
// create GPaths from the arrays in this file.
GPath *vector_create(const GPathInfo *info) {
  ensure_scaled();
  return gpath_create(info);
}
