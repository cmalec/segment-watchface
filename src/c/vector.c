#include <pebble.h>
#include "vector.h"

/*
 * Native Emery vector art. These coordinates are authored at the actual
 * 200x228 target size. No legacy-platform scale factor is applied at runtime.
 */

GPathInfo HealthFootPathInfo = {
  .num_points = 13,
  .points = (GPoint []) {{0,1}, {1,1}, {1,0}, {2,0}, {2,1}, {4,1}, {4,4}, {2,4}, {2,6}, {1,6}, {1,5}, {0,5}, {0,1}}
};
GPathInfo HealthHeelPathInfo = {
  .num_points = 4,
  .points = (GPoint []) {{1,9}, {2,9}, {2,11}, {1,11}}
};
GPathInfo HealthZee1PathInfo = {
  .num_points = 6,
  .points = (GPoint []) {{0,2}, {2,2}, {2,4}, {0,6}, {0,8}, {2,8}}
};
GPathInfo HealthZee2PathInfo = {
  .num_points = 6,
  .points = (GPoint []) {{4,1}, {8,1}, {8,2}, {4,6}, {4,8}, {8,8}}
};
GPathInfo HealthZee3PathInfo = {
  .num_points = 6,
  .points = (GPoint []) {{9,0}, {15,0}, {15,1}, {9,6}, {9,8}, {15,8}}
};
GPathInfo HealthHeartPathInfo = {
  .num_points = 13,
  .points = (GPoint []) {{0,5}, {2,1}, {5,2}, {6,4}, {8,2}, {11,1}, {13,5}, {6,13}, {0,5}, {0,5}, {0,5}, {0,5}, {0,5}}
};

GPathInfo BluetoothPathInfo = {
  .num_points = 18,
  .points = (GPoint []) {{0,1}, {2,4}, {4,4}, {4,0}, {5,0}, {8,2}, {5,5}, {8,8}, {5,11}, {4,11}, {4,6}, {2,6}, {0,9}, {2,6}, {4,6}, {4,4}, {2,4}, {0,1}}
};

GPathInfo ArrowLeftPathInfo = {
  .num_points = 5,
  .points = (GPoint []) {{0,2}, {5,0}, {5,6}, {0,4}, {0,2}}
};
GPathInfo ArrowRightPathInfo = {
  .num_points = 5,
  .points = (GPoint []) {{0,0}, {5,2}, {5,4}, {0,6}, {0,0}}
};
GPathInfo WaterResistOuterPathInfo = {
  .num_points = 9,
  .points = (GPoint []) {{0,2}, {2,0}, {54,0}, {56,2}, {56,12}, {48,20}, {6,20}, {0,13}, {0,2}}
};
GPathInfo BatteryBoltPathInfo = {
  .num_points = 13,
  .points = (GPoint []) {{5,5}, {8,5}, {8,4}, {11,4}, {11,2}, {11,5}, {16,5}, {13,5}, {13,6}, {11,6}, {11,8}, {11,5}, {15,5}}
};


