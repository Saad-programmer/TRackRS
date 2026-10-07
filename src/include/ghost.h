// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include <string>
#include <vector>
#include "vmath.h"

class PSSModel;
struct PVehiclePart;

///
/// @brief Recording and play back of ghost vehicles
///
class PGhost {
public:
  ///
  /// @brief Wheels of the ghost
  ///
  struct GhostWheel {
    // Position in world coordinates
    vec3f pos;
    // Orientation of the wheel
    quatf ori;
  };

  ///
  /// @brief State of the ghost vehicle
  ///
  struct GhostData {
    // Time stamp in seconds in game time
    float time;
    // Position in world coordinates
    vec3f pos;
    // Orientation of the vehicle
    quatf ori;
    // Status of the wheels
    std::vector<GhostWheel> wheel;
  };

  PGhost(float sampletime);
  void recordStart(const std::string &map, const std::string &vehicle);
  void recordSample(float delta, const PVehiclePart &part);
  void recordStop(float time);
  bool getReplayData(GhostData &data, std::string &vehicle) const;

private:
  PGhost();
  PGhost(const PGhost&);
  PGhost& operator=(const PGhost&);

  // Map related to the ghost car
  std::string mapname;
  // Name of the vehicle for recording
  std::string vehiclename;
  // Minimum time after which a sample is taken
  float sampletime;
  // To check if time since last has passed
  float lastsample;
  // Recorded data samples
  std::vector<GhostData> recordeddata;
  // Replay data samples
  std::vector<GhostData> replaydata;
  // Accumulated race time
  float racetime;
  // Ghost vehicle for replay
  std::string replayvehicle;
  // Total race time of replay vehicle
  float replaytime;
};
