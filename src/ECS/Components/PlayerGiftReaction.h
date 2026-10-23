#pragma once
#include <chrono>

namespace openblack::ecs::components
{
// Short visual response to a player delivery; does not replace the villager AI state.
struct PlayerGiftReaction
{
    std::chrono::steady_clock::time_point started;
    std::chrono::steady_clock::time_point until;
};
}
