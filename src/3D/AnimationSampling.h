/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <glm/common.hpp>
#include <glm/mat4x4.hpp>
#include "L3DAnim.h"

namespace openblack::animation
{
inline std::vector<glm::mat4> SampleLoop(std::span<const L3DAnim::Frame> frames, uint32_t duration, uint32_t time)
{
    if (frames.empty()) return {};
    if (!duration || frames.size() == 1) return frames.front().bones;
    const auto tick = time % duration;
    const auto next = std::upper_bound(frames.begin(), frames.end(), tick,
        [](uint32_t value, const auto& frame) { return value < frame.time; });
    if (next == frames.begin()) return frames.front().bones;
    const auto& before = *(next - 1);
    const auto& after = next == frames.end() ? frames.front() : *next;
    const auto end = next == frames.end() ? duration : after.time;
    if (before.bones.size() != after.bones.size() || end <= before.time) return before.bones;
    const float factor = glm::clamp(float(tick - before.time) / float(end - before.time), 0.0f, 1.0f);
    std::vector<glm::mat4> result(before.bones.size());
    for (size_t bone = 0; bone < result.size(); ++bone)
        for (size_t column = 0; column < 4; ++column)
            result[bone][column] = glm::mix(before.bones[bone][column], after.bones[bone][column], factor);
    return result;
}

inline bool ComposeHierarchy(std::vector<glm::mat4>& bones, std::span<const uint32_t> parents)
{
    if (bones.size() != parents.size()) return false;
    for (size_t i = 0; i < bones.size(); ++i)
    {
        if (parents[i] != std::numeric_limits<uint32_t>::max())
        {
            if (parents[i] >= i) return false; // Reject cycles and unsupported parent ordering.
            bones[i] = bones[parents[i]] * bones[i];
        }
        for (size_t column = 0; column < 4; ++column)
            for (size_t row = 0; row < 4; ++row)
                if (!std::isfinite(bones[i][column][row])) return false;
    }
    return true;
}
}
