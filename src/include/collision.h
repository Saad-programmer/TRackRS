// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include "psim.h"
#include "vehicle.h"

class PTerrainFoliage;

///
/// @brief Handling of collisions with world objects
///
class PCollision {
public:
  PCollision(const std::vector<vehicle_clip_s> &clip, PReferenceFrame &ref_world);
  const std::vector<PTerrainFoliage> checkContact(const std::vector<PTerrainFoliage> *foliage) const;
  bool towardsContact(const vec3f &body, const vec3f &contact, const vec3f &diff) const;
  const vec3f &getCrashPoint(const vec3f &body, const PTerrainFoliage &foliage);

private:
  PCollision();
  PCollision(const PCollision&);
  PCollision& operator=(const PCollision&);

  void calcmin(const vec3f &a);
  void calcmax(const vec3f &a);

  // 3D minimum position of AABB box
  vec3f boxmin;
  // 3D maximum position of AABB box
  vec3f boxmax;
  // Position to apply crash force
  vec3f crashpoint;
};
