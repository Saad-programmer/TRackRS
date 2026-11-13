// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Car Definition v2 (`car.json`, schema "trackrs.car/2", master prompt §4.3).
 *
 * Purpose: one data-driven, SI-unit description of a car (mass, torque curve
 * in N·m, gearbox ratios and final drive, layout and torque split, tyres,
 * aero, brakes) with explicit licensing, sourcing and `verified` flags, plus a
 * validator and a two-way adapter to the legacy `.vehicle` XML that the
 * current physics (PSim) consumes. Phase 6 replaces the legacy physics; until
 * then the adapter is the only bridge.
 *
 * Ownership: value types. Thread-safety: pure functions. No GL, no SDL.
 * Honesty rules enforced by validate(): a value may only be `verified` when
 * at least one source URL is listed; a car without its own licensed mesh must
 * name a fictional fallback body and is labelled "(placeholder model)".
 */
#include <map>
#include <string>
#include <string_view>
#include <vector>
namespace trackrs::content {

struct TorquePoint {
  float rpm = 0, nm = 0;
};

struct CarDefinition {
  std::string schema = "trackrs.car/2";
  std::string id, displayName, marque, carClass, era;
  int year = 0;
  struct License {
    std::string data = "GPL-2.0-or-later"; ///< SPDX id of this data file
    std::string model;                     ///< SPDX id of the mesh ("" = no mesh)
    std::string author, sourceUrl, trademarkNotice;
  } license;
  std::map<std::string, bool> verified;    ///< mass, engine, gearbox, tyres, aero, brakes
  std::vector<std::string> sources;        ///< https:// URLs backing verified values
  std::string provenance;                  ///< how unverified values were obtained
  struct Body {
    std::string mesh;                      ///< own licensed mesh ("" when absent)
    std::string fallback;                  ///< fictional body/vehicle id used when mesh is ""
    float importScale = 1;
  } body;
  float massKg = 0, frontWeight = .5f;
  std::string engineLayout;
  float idleRpm = 0, redlineRpm = 0;
  std::vector<TorquePoint> torque;         ///< N·m at the crank, full load
  std::string driveLayout;                 ///< FWD, RWD or AWD
  std::vector<float> gearRatios;           ///< forward gears, 1st first (descending)
  float finalDrive = 1, reverseRatio = 0, shiftTime = .2f, frontTorqueShare = 0;
  std::string gearboxDerivation;           ///< "sourced", "legacy" or "synthetic: ..."
  float tyreRadius = 0, tyreWidth = 0;
  float cd = 0, frontalArea = 0, clFront = 0, clRear = 0;
  float brakeBiasFront = .6f;
  float claimedTopSpeedKmh = 0, claimedZeroTo100 = 0; ///< optional (0 = unknown), unverified unless sourced
  float damageVisualGain = 0; ///< optional "damage.visual_gain" override of graphics.damagegain (0 = use the setting)
  bool legacy = false;                     ///< produced by the legacy adapter (fictional car)
  std::string legacyFile;                  ///< source .vehicle for legacy cars
};

struct Validation {
  std::vector<std::string> errors, warnings;
  bool ok() const { return errors.empty(); }
};

CarDefinition parseCar(std::string_view json);
std::string writeCar(const CarDefinition &car); ///< canonical JSON (2-space indent)
Validation validate(const CarDefinition &car);
bool isPlaceholder(const CarDefinition &car);
/// Name shown in menus: "(placeholder model)" is appended when no own mesh.
std::string menuName(const CarDefinition &car);

/// Crank torque (N·m) at rpm: linear interpolation, 0 outside [first, redline].
float torqueAt(const CarDefinition &car, float rpm);
struct Peak {
  float value = 0, rpm = 0;
};
Peak peakTorque(const CarDefinition &car);
Peak peakPowerKw(const CarDefinition &car); ///< from a 10 rpm sweep of the curve
/// Road speed (km/h) at `rpm` in forward gear `gear` (0-based).
float speedKmh(const CarDefinition &car, unsigned gear, float rpm);

// ---- legacy adapter --------------------------------------------------------
/// Legacy power units per watt. Legacy `powerscale` is not SI: wheel spin is
/// integrated from torque without wheel inertia. Calibrated once from the
/// tuned lancer_evo (powerscale 26000 for its documented ~300 bhp ≈ 223.7 kW).
/// Phase 6 (real drivetrain with inertia) removes this constant.
constexpr float LegacyPowerPerWatt = 26000.f / 223710.f;
/// Legacy .vehicle XML -> Car v2 (fictional cars; physics keeps the XML).
CarDefinition fromLegacyVehicle(std::string_view xml, std::string_view id, std::string_view file);
/// Car v2 + fallback body .vehicle XML -> legacy .vehicle XML for PSim:
/// replaces name, mass, engine curve (power = torque·ω·LegacyPowerPerWatt,
/// cut just above redline), gearbox (wheel rev per engine rev =
/// 1/(ratio·final)), drive split, wheel radius (+ model scale), brake bias.
std::string toLegacyVehicle(const CarDefinition &car, std::string_view fallbackXml);
} // namespace trackrs::content
