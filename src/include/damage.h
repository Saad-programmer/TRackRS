// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include "vmath.h"
#include <vector>

class PReferenceFrame;
struct vehicle_clip_s;

///
/// @brief Damage indicator in racing UI
///
class PDamage {
public:
  ///
  /// @brief Side of the vehicle colliding with object
  ///
  enum DamageSide {
    DamageFrontLeft = 0,
    DamageFrontRight,
    DamageRearLeft,
    DamageRearRight,
    DamageSideSize
  };

  void setClip(const std::vector<vehicle_clip_s> &clip);
  void addDamage(const vec3f &crashpoint, float increment, PReferenceFrame &ref_world);
  float getDamage(DamageSide damageside);
  float getDamage(const vec3f &position);
  /// Damage of one side clamped to 0..1, without the HUD flash side effect
  /// of getDamage(DamageSide). For renderers (Phase 4c damage visuals).
  float level(DamageSide side) const;

private:
  // Damage of each vehicle side
  float damage[DamageSide::DamageSideSize];
  // Center position of each vehicle side
  vec3f center[DamageSide::DamageSideSize];
  // Indicator flashes on impact
  bool flash[DamageSide::DamageSideSize];
};
