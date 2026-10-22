/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/*
 * The layout of a ANM File is as follows:
 *
 * - 84 byte header
 *         animation name, containing 32 chars
 *         1 unknown 32-bit int, likely a short // TODO
 *         5 unknown floats // TODO
 *         frame count
 *         1 unknown 32-bit int, likely a bool // TODO
 *         animation duration
 *         2 unknown 32-bit ints, likely bools // TODO
 *         keyframe start offset - offset of keyframe offset block (see below)
 *         1 unknown 32-bit int, likely a short // TODO
 *
 * ------------------------ start of keyframe offsets block --------------------
 *
 * - 4 bytes * frame count, each record contains:
 *         offset into keyframe pointer block (see below)
 *
 * ------------------------ start of keyframe pointer block --------------------
 *
 * - 4 bytes offset containing:
 *         offset into bone offset block (see below)
 *
 * ------------------------ start of bone offset block -------------------------
 *
 * - 4 bytes offset containing:
 *         offset into bone block (see below)
 *
 * ------------------------ start of bone block --------------------------------
 *
 * - 8 byte header containing:
 *         number of bones in the block
 *         timestamp of animation
 * - Bone matrix list of length of number of bones each record containing:
 *         a 4x3 matrix
 *
 */

#include "ANMFile.h"

#include <cassert>
#include <cstring>

#include <fstream>
#include <utility>

using namespace openblack::anm;

namespace
{
// Adapted from https://stackoverflow.com/a/13059195/10604387
//          and https://stackoverflow.com/a/46069245/10604387
struct membuf: std::streambuf
{
	membuf(char const* base, size_t size)
	{
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
	std::streampos seekoff(off_type off, std::ios_base::seekdir way, [[maybe_unused]] std::ios_base::openmode which) override
	{
		if (way == std::ios_base::cur)
		{
			gbump(static_cast<int>(off));
		}
		else if (way == std::ios_base::end)
		{
			setg(eback(), egptr() + off, egptr());
		}
		else if (way == std::ios_base::beg)
		{
			setg(eback(), eback() + off, egptr());
		}
		return gptr() - eback();
	}

	std::streampos seekpos([[maybe_unused]] pos_type pos, [[maybe_unused]] std::ios_base::openmode which) override
	{
		return seekoff(pos - static_cast<off_type>(0), std::ios_base::beg, which);
	}
};
struct imemstream: virtual membuf, std::istream
{
	imemstream(char const* base, size_t size)
	    : membuf(base, size)
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	{
	}
};
} // namespace

std::string_view openblack::anm::ResultToStr(ANMResult result)
{
	switch (result)
	{
	case ANMResult::Success:
		return "Success";
	case ANMResult::ErrCantOpen:
		return "Could not open file.";
	case ANMResult::ErrFileTooSmall:
		return "File too small to be a valid ANM file.";
	}
	std::unreachable();
}

ANMFile::ANMFile() noexcept = default;
ANMFile::~ANMFile() noexcept = default;

ANMResult ANMFile::ReadFile(std::istream& stream) noexcept
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(ANMHeader))
	{
		return ANMResult::ErrFileTooSmall;
	}

	// First 84 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(ANMHeader));

    if (!stream || _header.framesBase > fsize ||
        _header.frameCount > (fsize - _header.framesBase) / sizeof(uint32_t))
        return ANMResult::ErrFileTooSmall;
    _keyframes.resize(_header.frameCount);
    for (uint32_t i = 0; i < _header.frameCount; ++i)
    {
        uint32_t cursor = _header.framesBase + i * sizeof(uint32_t);
        for (int level = 0; level < 3; ++level)
        {
            if (cursor > fsize - sizeof(uint32_t)) return ANMResult::ErrFileTooSmall;
            stream.seekg(cursor);
            stream.read(reinterpret_cast<char*>(&cursor), sizeof(cursor));
            if (!stream) return ANMResult::ErrFileTooSmall;
        }
        if (cursor > fsize - 2 * sizeof(uint32_t)) return ANMResult::ErrFileTooSmall;
        stream.seekg(cursor);
        uint32_t boneCount = 0, boneDataOffset = 0;
        stream.read(reinterpret_cast<char*>(&boneCount), sizeof(boneCount));
        stream.read(reinterpret_cast<char*>(&boneDataOffset), sizeof(boneDataOffset));
        if (!stream || boneDataOffset > fsize || boneCount > (fsize - boneDataOffset) / sizeof(ANMBone))
            return ANMResult::ErrFileTooSmall;
        // The second word points to matrices, not a timestamp. Packed frames
        // uniformly divide the clip duration stored at header offset 0x20.
        _keyframes[i].time = static_cast<uint32_t>(static_cast<uint64_t>(i) * GetDuration() / _header.frameCount);
        stream.seekg(boneDataOffset);
        _keyframes[i].bones.resize(boneCount);
        stream.read(reinterpret_cast<char*>(_keyframes[i].bones.data()), boneCount * sizeof(ANMBone));
        if (!stream) return ANMResult::ErrFileTooSmall;
    }

	_isLoaded = true;

	return ANMResult::Success;
}

ANMResult ANMFile::WriteFile([[maybe_unused]] std::ostream& stream) const noexcept
{
	assert(!_isLoaded);
	stream.write(reinterpret_cast<const char*>(&_header), sizeof(_header));

	return ANMResult::Success;
}

ANMResult ANMFile::Open(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);

	std::ifstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return ANMResult::ErrCantOpen;
	}

	return ReadFile(stream);
}

ANMResult ANMFile::Open(const std::vector<uint8_t>& buffer) noexcept
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	return ReadFile(stream);
}

ANMResult ANMFile::Write(const std::filesystem::path& filepath) noexcept
{
	assert(!_isLoaded);

	std::ofstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return ANMResult::ErrCantOpen;
	}

	return WriteFile(stream);
}
