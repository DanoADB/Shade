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
#include <limits>
#include <new>
#include <utility>

#include "RendererInterface.h"
#include <GLES3/gl3.h>
#include <emscripten/html5.h>
#include <glm/gtc/type_ptr.hpp>
#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "Camera/Camera.h"
#include "Graphics/Mesh.h"
#include "Locator.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::graphics
{

// First browser pass: engine terrain geometry with diagnostic shading. Other resources retain metadata only.
class RendererWebGl final: public RendererInterface
{
public:
    static std::unique_ptr<RendererInterface> Create() noexcept
    {
        EmscriptenWebGLContextAttributes attributes;
        emscripten_webgl_init_context_attributes(&attributes);
        attributes.majorVersion = 2;
        attributes.alpha = false;
        auto context = emscripten_webgl_create_context("#canvas", &attributes);
        if (context <= 0 || emscripten_webgl_make_context_current(context) != EMSCRIPTEN_RESULT_SUCCESS)
            return nullptr;
        auto renderer = std::make_unique<RendererWebGl>();
        renderer->_context = context;
        const char* vertex = R"(#version 300 es
layout(location=0) in vec3 position;
layout(location=1) in vec4 light;
uniform mat4 viewProjection;
uniform vec2 offset;
out vec3 color;
void main() {
    vec3 world = position + vec3(offset.x, 0.0, offset.y);
    gl_Position = viewProjection * vec4(world, 1.0);
    color = mix(vec3(0.18,0.33,0.12),vec3(0.55,0.48,0.32),clamp(world.y/160.0,0.0,1.0)) * (0.35+0.65*light.r);
})";
        const char* fragment = R"(#version 300 es
precision highp float;
in vec3 color;
out vec4 outputColor;
void main() { outputColor = vec4(color,1.0); }
)";
        GLuint shaders[2] = {glCreateShader(GL_VERTEX_SHADER), glCreateShader(GL_FRAGMENT_SHADER)};
        const char* sources[2] = {vertex, fragment};
        renderer->_program = glCreateProgram();
        for (int i=0;i<2;++i) {
            glShaderSource(shaders[i],1,&sources[i],nullptr);
            glCompileShader(shaders[i]);
            GLint compiled=0; glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&compiled);
            if (!compiled) {
                char log[2048]; glGetShaderInfoLog(shaders[i],sizeof(log),nullptr,log);
                emscripten_log(EM_LOG_ERROR,"Terrain shader: %s",log);
                for (auto shader : shaders) glDeleteShader(shader);
                return nullptr;
            }
            glAttachShader(renderer->_program,shaders[i]);
        }
        glLinkProgram(renderer->_program);
        for (auto shader : shaders) glDeleteShader(shader);
        GLint linked=0; glGetProgramiv(renderer->_program,GL_LINK_STATUS,&linked);
        if (!linked) return nullptr;
        glGenVertexArrays(1,&renderer->_vao);
        emscripten_log(EM_LOG_CONSOLE,"Shade WebGL2 terrain renderer initialized (materials/entities pending)");
        return renderer;
    }
    ~RendererWebGl() noexcept override {
        if (_program) glDeleteProgram(_program);
        if (_vao) glDeleteVertexArrays(1,&_vao);
        if (_context > 0) emscripten_webgl_destroy_context(_context);
    }
	[[nodiscard]] void* GetDevice() const noexcept final { return nullptr; }
	void DrawScene(const DrawSceneDesc& desc) noexcept final {
        if (desc.viewId != RenderPass::Main || !desc.camera) return;
        int width=0,height=0;
        emscripten_get_canvas_element_size("#canvas",&width,&height);
        glViewport(0,0,width,height);
        glClearColor(0.45f,0.65f,0.85f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        if (!desc.drawIsland || !Locator::terrainSystem::has_value()) return;
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDisable(GL_CULL_FACE);
        glUseProgram(_program); glBindVertexArray(_vao);
        const auto matrix=desc.camera->GetViewProjectionMatrix(Camera::Projection::Normal);
        glUniformMatrix4fv(glGetUniformLocation(_program,"viewProjection"),1,GL_FALSE,glm::value_ptr(matrix));
        for (const auto& block: Locator::terrainSystem::value().GetBlocks()) {
            const auto& buffer=block.GetMesh().GetVertexBuffer();
            if (!buffer.handle.id) continue;
            glBindBuffer(GL_ARRAY_BUFFER,static_cast<GLuint>(buffer.handle.id));
            glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,buffer.strideBytes,nullptr);
            glEnableVertexAttribArray(1); glVertexAttribPointer(1,4,GL_UNSIGNED_BYTE,GL_TRUE,buffer.strideBytes,reinterpret_cast<void*>(36));
            const auto offset=block.GetMapPosition();
            glUniform2f(glGetUniformLocation(_program,"offset"),offset.x,offset.y);
            glDrawArrays(GL_TRIANGLES,0,buffer.vertexCount);
        }
        glBindVertexArray(0);
    }
	void Frame() noexcept final {}
	void RequestScreenshot(const std::filesystem::path&) noexcept final {}
	[[nodiscard]] bool GetDebug() const noexcept final { return _debug; }
	void SetDebug(bool value) noexcept final { _debug = value; }
	[[nodiscard]] bool GetProfile() const noexcept final { return _profile; }
	void SetProfile(bool value) noexcept final { _profile = value; }
	void DrawMesh(const L3DMesh&, const L3DMeshSubmitDesc&, uint8_t) const noexcept final {}

	[[nodiscard]] VertexBufferUniquePtr CreateVertexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                       VertexDecl decl) noexcept final
	{
		// Match the packed vertex layout used by the GPU renderer.
		static constexpr std::array<std::array<uint32_t, 4>, 3> k_Strides = {{{4, 4, 4, 4}, {4, 4, 8, 8}, {4, 8, 12, 16}}};
		uint32_t stride = 0;
		for (const auto& attrib : decl)
		{
			if (attrib.num < 1 || attrib.num > 4 || static_cast<size_t>(attrib.type) >= k_Strides.size())
			{
				return nullptr;
			}
			stride += k_Strides.at(static_cast<size_t>(attrib.type)).at(attrib.num - 1);
		}
		if (stride == 0 || memory.empty() || memory.size_bytes() > std::numeric_limits<uint32_t>::max() ||
		    memory.size_bytes() % stride != 0)
		{
			return nullptr;
		}
        GLuint handle=0;
        if (name == "LandBlock") {
            glGenBuffers(1,&handle);
            glBindBuffer(GL_ARRAY_BUFFER,handle);
            glBufferData(GL_ARRAY_BUFFER,memory.size_bytes(),memory.data(),GL_STATIC_DRAW);
        }
        return {new (std::nothrow) VertexBuffer {std::move(name),static_cast<uint32_t>(memory.size_bytes()/stride),stride,{handle}},
                [](VertexBuffer* buffer) { const auto id=static_cast<GLuint>(buffer->handle.id); if(id) glDeleteBuffers(1,&id); delete buffer; }};
	}

	[[nodiscard]] IndexBufferUniquePtr CreateIndexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                     IndexBuffer::Type type) noexcept final
	{
		const auto stride = IndexBuffer::GetTypeSize(type);
		if (memory.empty() || memory.size_bytes() > std::numeric_limits<uint32_t>::max() || memory.size_bytes() % stride != 0)
		{
			return nullptr;
		}
		return {new (std::nothrow) IndexBuffer {std::move(name), static_cast<uint32_t>(memory.size_bytes() / stride), type, {}},
		        [](IndexBuffer* buffer) { delete buffer; }};
	}

	[[nodiscard]] Texture2dUniquePtr CreateTexture2d(std::string name, std::span<const uint8_t>, glm::u16vec2 resolution,
	                                                 uint16_t layers, TextureFormat format, Wrapping, Filter) noexcept final
	{
		return {new (std::nothrow) Texture2d {std::move(name), {}, {}, resolution, layers, format},
		        [](Texture2d* texture) { delete texture; }};
	}

	[[nodiscard]] std::unique_ptr<FrameBuffer> CreateFrameBuffer(std::string name, glm::u16vec2 resolution,
	                                                             TextureFormat colorFormat,
	                                                             std::optional<TextureFormat> depthStencilFormat) noexcept final
	{
		return std::make_unique<FrameBuffer>(std::move(name), resolution, colorFormat, depthStencilFormat, TextureHandle {},
		                                     SamplerHandle {}, TextureHandle {}, [] {});
	}

private:
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE _context {};
    GLuint _program {}, _vao {};
	bool _debug {};
	bool _profile {};
};

} // namespace openblack::graphics
