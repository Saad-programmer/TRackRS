// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Renderer consumes borrowed immutable vehicle snapshots. No world ownership.
 * Main GL thread only; all resources must outlive each call. No frame allocations.
 */
class PApp; class PTerrain; class PVehicle; class PVehicleType; struct PVehiclePart; struct PVehicleTypePart;
enum class TerrainType;
namespace trackrs {
class Renderer {
public:
    static void vehiclePart(PApp& context, const PVehicleType& type,
        const PVehiclePart& part, const PVehicleTypePart& typepart, float alpha);
    /// Phase 4b: draws the vehicle with the PBR CarRenderer when the PBR world
    /// scene is active and the car renderer is enabled. Returns false (nothing
    /// drawn) otherwise, so the caller keeps the legacy path. `surface` is the
    /// road surface under the car (dirt accumulation).
    static bool car(const PVehicle& vehicle, TerrainType surface, float time);
    /// Feeds one simulation step of the vehicle's state (speed, brake,
    /// surface) to the car renderer's wear model (dust, mud, brake heat).
    static void observeCar(const PVehicle& vehicle, TerrainType surface, float dt);
    /// Feeds one simulation step of every wheel contact to the persistent
    /// ground tracks (amendment A3.3). Visual only; `car` numbers the trails.
    static void observeTracks(const PVehicle& vehicle, unsigned car, PTerrain& terrain);
};
}
