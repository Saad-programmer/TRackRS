// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/** Application-owned state; no engine, UI, world or GL dependencies.
 * Single application thread. O(1) typed transitions preserve legacy dispatch.
 */
#include <cstdint>
namespace trackrs {
enum class AppState : std::uint8_t { Load1 = 1, Load2 = 2, Load3 = 3, Menu = 10, ChooseVehicle = 11, Race = 12, End = 13 };
class GameStateMachine {
    AppState state_ = AppState::Load1;
    std::uint64_t generation_ = 0;
public:
    operator AppState() const noexcept { return state_; }
    GameStateMachine& operator=(AppState next) noexcept {
        if (state_ != next) { state_ = next; ++generation_; }
        return *this;
    }
    std::uint64_t generation() const noexcept { return generation_; }
};
}
