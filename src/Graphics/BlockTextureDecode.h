/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once
#include <array>
#include <cstring>
#include <span>
#include <vector>
#include "Texture2d.h"
namespace openblack::graphics::detail
{
inline uint16_t ReadU16(const uint8_t* data)
{
	return static_cast<uint16_t>(data[0] | (data[1] << 8));
}

inline uint32_t ReadU32(const uint8_t* data)
{
	return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) | (static_cast<uint32_t>(data[2]) << 16) |
	       (static_cast<uint32_t>(data[3]) << 24);
}

inline std::array<uint8_t, 4> ExpandRgb565(uint16_t color)
{
	const auto r = static_cast<uint8_t>((color >> 11) & 0x1F);
	const auto g = static_cast<uint8_t>((color >> 5) & 0x3F);
	const auto b = static_cast<uint8_t>(color & 0x1F);
	return {static_cast<uint8_t>((r << 3) | (r >> 2)), static_cast<uint8_t>((g << 2) | (g >> 4)),
	        static_cast<uint8_t>((b << 3) | (b >> 2)), 0xFF};
}

// Decodes the 8 byte color part of a BC1/BC2/BC3 block into 16 RGBA texels
inline void DecodeColorBlock(const uint8_t* block, bool allowPunchThrough, std::array<std::array<uint8_t, 4>, 16>& texels)
{
	const auto c0 = ReadU16(block);
	const auto c1 = ReadU16(block + 2);
	std::array<std::array<uint8_t, 4>, 4> palette {ExpandRgb565(c0), ExpandRgb565(c1)};
	const bool fourColor = !allowPunchThrough || c0 > c1;
	for (size_t i = 0; i < 3; ++i)
	{
		const auto a = palette[0].at(i);
		const auto b = palette[1].at(i);
		if (fourColor)
		{
			palette[2].at(i) = static_cast<uint8_t>((2 * a + b) / 3);
			palette[3].at(i) = static_cast<uint8_t>((a + 2 * b) / 3);
		}
		else
		{
			palette[2].at(i) = static_cast<uint8_t>((a + b) / 2);
			palette[3].at(i) = 0;
		}
	}
	palette[2][3] = 0xFF;
	palette[3][3] = fourColor ? 0xFF : 0;
	const auto indices = ReadU32(block + 4);
	for (size_t i = 0; i < texels.size(); ++i)
	{
		texels.at(i) = palette.at((indices >> (2 * i)) & 0x3);
	}
}

// Mobile GPUs such as Mali cannot sample BC textures, so those are expanded to RGBA8 on upload
inline std::vector<uint8_t> DecodeBlockCompressed(TextureFormat format, std::span<const uint8_t> memory, uint16_t width,
                                           uint16_t height, uint16_t layers)
{
	const size_t blockSize = format == TextureFormat::BlockCompression1 ? 8 : 16;
	const size_t blocksX = (width + 3) / 4;
	const size_t blocksY = (height + 3) / 4;
	const size_t layerStride = static_cast<size_t>(width) * height * 4;
	std::vector<uint8_t> result(layerStride * layers);
	std::array<std::array<uint8_t, 4>, 16> texels {};
	const auto* block = memory.data();
	for (size_t layer = 0; layer < layers; ++layer)
	{
		for (size_t by = 0; by < blocksY; ++by)
		{
			for (size_t bx = 0; bx < blocksX; ++bx, block += blockSize)
			{
				if (format == TextureFormat::BlockCompression1)
				{
					DecodeColorBlock(block, true, texels);
				}
				else
				{
					DecodeColorBlock(block + 8, false, texels);
					if (format == TextureFormat::BlockCompression2)
					{
						for (size_t i = 0; i < texels.size(); ++i)
						{
							const auto alpha = static_cast<uint8_t>((block[i / 2] >> (4 * (i % 2))) & 0xF);
							texels.at(i)[3] = static_cast<uint8_t>(alpha * 0x11);
						}
					}
					else
					{
						const auto a0 = block[0];
						const auto a1 = block[1];
						std::array<uint8_t, 8> alphas {a0, a1, 0, 0, 0, 0, 0, 0xFF};
						const size_t steps = a0 > a1 ? 7 : 5;
						for (size_t i = 1; i < steps; ++i)
						{
							alphas.at(i + 1) = static_cast<uint8_t>(((steps - i) * a0 + i * a1) / steps);
						}
						uint64_t indices = 0;
						for (size_t i = 0; i < 6; ++i)
						{
							indices |= static_cast<uint64_t>(block[2 + i]) << (8 * i);
						}
						for (size_t i = 0; i < texels.size(); ++i)
						{
							texels.at(i)[3] = alphas.at((indices >> (3 * i)) & 0x7);
						}
					}
				}
				for (size_t i = 0; i < texels.size(); ++i)
				{
					const size_t x = bx * 4 + i % 4;
					const size_t y = by * 4 + i / 4;
					if (x < width && y < height)
					{
						std::memcpy(&result[layer * layerStride + (y * width + x) * 4], texels.at(i).data(), 4);
					}
				}
			}
		}
	}
	return result;
}
}
