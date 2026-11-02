// SPDX-License-Identifier: GPL-2.0-or-later
/** Intentional application leak used only by the ASan policy regression test.
 * Owns an intentionally unreleased block; main-thread-only, never game-linked.
 */
#include <cstdio>
__attribute__((noinline)) void TrackRSOriginLeakCanary() {
    auto* allocation = new int[17];
    allocation[0] = 7;
    std::fprintf(stderr, "intentional application canary: %p\n", static_cast<void*>(allocation));
}
int main() { TrackRSOriginLeakCanary(); }
