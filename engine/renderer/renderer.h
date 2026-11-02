// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Renderer consumes borrowed immutable vehicle snapshots. No world ownership.
 * Main GL thread only; all resources must outlive each call. No frame allocations.
 */
class PApp; class PVehicleType; struct PVehiclePart; struct PVehicleTypePart;
namespace trackrs {
class Renderer {
public:
    static void vehiclePart(PApp& context, const PVehicleType& type,
        const PVehiclePart& part, const PVehicleTypePart& typepart, float alpha);
};
}
