/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "RendererInterface.h"

#define LOCATOR_IMPLEMENTATIONS

#include "RendererNoOp.h"
#ifdef __EMSCRIPTEN__
#include "RendererWebGl.h"
#else
#include "RendererSdl3Gpu.h"
#endif

using namespace openblack::graphics;

std::unique_ptr<RendererInterface> RendererInterface::Create(GraphicsBackend backend, bool vsync) noexcept
{
	if (backend == GraphicsBackend::Noop)
	{
		return std::make_unique<RendererNoOp>();
	}
	#ifdef __EMSCRIPTEN__
	(void)vsync;
	return RendererWebGl::Create();
#else
	return RendererSdl3Gpu::Create(vsync);
#endif
}
