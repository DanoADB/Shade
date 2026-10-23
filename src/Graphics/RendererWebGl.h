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
#include "BlockTextureDecode.h"
#include <GLES3/gl3.h>
#include <emscripten/html5.h>
#include <glm/gtc/type_ptr.hpp>
#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "3D/OceanInterface.h"
#include "Camera/Camera.h"
#include "Graphics/Mesh.h"
#include "Locator.h"
#include "3D/L3DMesh.h"
#include "3D/AnimationSampling.h"
#include "3D/L3DSubMesh.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "Resources/ResourcesInterface.h"
#include "Debug/DebugGuiInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::graphics
{

// WebGL2 backend for the shared engine scene and original assets.
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
layout(location=2) in vec3 weight;
layout(location=3) in uvec3 firstMaterial;
layout(location=4) in uvec3 secondMaterial;
layout(location=5) in vec3 materialBlend;
layout(location=6) in float waterAlpha;
uniform mat4 viewProjection;
uniform vec2 offset;
uniform mat4 view;
uniform vec4 islandExtent;
out vec2 uv;
out vec3 weights;
flat out uvec3 first;
flat out uvec3 second;
out vec3 blend;
out float lighting;
out float alpha;
out float distanceToCamera;
out vec2 footprintUv;
void main() {
    vec3 world = position + vec3(offset.x, 0.0, offset.y);
    gl_Position = viewProjection * vec4(world, 1.0);
    uv = position.zx / 160.0;
    weights=weight; first=firstMaterial; second=secondMaterial;
    blend=materialBlend;lighting=light.r;alpha=waterAlpha;
    distanceToCamera=(view*vec4(world,1.0)).z;
    footprintUv=(world.xz-islandExtent.xy)/(islandExtent.zw-islandExtent.xy);
})";
        const char* fragment = R"(#version 300 es
precision highp float;
precision highp sampler2DArray;
in vec2 uv;
in vec3 weights;
flat in uvec3 first;
flat in uvec3 second;
in vec3 blend;
in float lighting;
in float alpha;
in float distanceToCamera;
in vec2 footprintUv;
uniform sampler2DArray materials;
uniform sampler2D bumpMap;
uniform sampler2D smallBump;
uniform sampler2D footprint;
uniform vec3 skyAndBump;
out vec4 outputColor;
void main() {
    if(alpha == 0.0) discard;
    vec3 color=vec3(0.0);
    for(int i=0;i<3;++i) {
        vec3 a=texture(materials,vec3(uv,float(first[i]))).rgb;
        vec3 b=texture(materials,vec3(uv,float(second[i]))).rgb;
        color+=mix(a,b,blend[i])*weights[i];
    }
    color*=mix(1.0,texture(bumpMap,uv).r*2.0,skyAndBump.y);
    if(distanceToCamera<200.0)color*=1.0-texture(smallBump,uv*10.0).r*(1.0-distanceToCamera/200.0)*skyAndBump.z;
    vec4 decal=texture(footprint,footprintUv);
    color=mix(color,decal.rgb,decal.a);
    color*=mix(0.25,clamp(lighting*2.0,0.5,1.0),skyAndBump.x/2.0);
    outputColor=vec4(color,alpha);
}
)";
        renderer->_program=CompileProgram(vertex,fragment);
        const char* objectVertex=R"(#version 300 es
layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
layout(location=2) in vec3 normal;
layout(location=3) in ivec2 joint;
uniform mat4 viewProjection;
uniform mat4 model;
uniform mat4 bones[64];
uniform int boneCount;
uniform sampler2D heightMap;
uniform vec4 islandExtent;
uniform bool morphWithTerrain;
uniform bool vegetation;
uniform vec4 sway;
out vec2 uv;
out vec3 worldPosition;
out vec3 worldNormal;
void main() {
    mat4 bone=mat4(1.0);
    if(joint.x>=0 && joint.x<boneCount) bone=bones[joint.x];
    vec4 world=model*bone*vec4(position,1.0);
    vec3 base=(model*vec4(0.0,0.0,0.0,1.0)).xyz;
    if(vegetation){
        vec2 direction=length(sway.xy)>0.0?normalize(sway.xy):vec2(0.0);
        world.xz+=direction*sin(sway.z*0.08*sway.w)*position.y;
    }
    if(morphWithTerrain||vegetation){
        vec2 point=vegetation?base.xz:world.xz;
        vec2 sampleUv=(point-islandExtent.xy)/(islandExtent.zw-islandExtent.xy);
        world.y+=textureLod(heightMap,sampleUv,0.0).r*170.85-base.y;
    }
    gl_Position=viewProjection*world;
    worldPosition=world.xyz;worldNormal=normal;
    uv=texcoord;
})";
        const char* objectFragment=R"(#version 300 es
precision highp float;
in vec2 uv;
in vec3 worldPosition;
in vec3 worldNormal;
uniform sampler2D diffuse;
uniform bool textured;
uniform float alphaCutoff;
uniform float skyType;
uniform bool vegetation;
out vec4 outputColor;
void main() {
    vec4 color=textured ? texture(diffuse,uv) : vec4(0.7,0.65,0.5,1.0);
    if(color.a<alphaCutoff) discard;
    float brightness=skyType/2.0;
    float ambient=vegetation?brightness*0.25+0.75:brightness*0.25+0.25;
    vec3 lightDirection=normalize(vec3(-4000.0,1300.0,-1435.0)-worldPosition);
    float lighting=ambient+brightness*max(dot(worldNormal,lightDirection),0.0)*(vegetation?0.25:0.75);
    outputColor=vec4(color.rgb*lighting,color.a);
})";
        renderer->_objectProgram=CompileProgram(objectVertex,objectFragment);
        const char* skyVertex=R"(#version 300 es
layout(location=0) in vec3 position;
layout(location=1) in vec2 texcoord;
uniform mat4 viewProjection;
out vec2 uv;
void main(){gl_Position=viewProjection*vec4(position,1.0);uv=texcoord;}
)";
        const char* skyFragment=R"(#version 300 es
precision highp float;
precision highp sampler2DArray;
in vec2 uv;
uniform sampler2DArray skies;
uniform vec2 skyData;
out vec4 outputColor;
void main(){
    vec2 value=clamp(skyData,vec2(0.0),vec2(2.0));
    vec2 a=floor(value),b=min(a+1.0,vec2(2.0)),t=fract(value);
    vec4 aa=texture(skies,vec3(uv,a.x+3.0*a.y));
    vec4 ab=texture(skies,vec3(uv,b.x+3.0*a.y));
    vec4 ba=texture(skies,vec3(uv,a.x+3.0*b.y));
    vec4 bb=texture(skies,vec3(uv,b.x+3.0*b.y));
    outputColor=mix(mix(aa,ba,t.y),mix(ab,bb,t.y),t.x);
}
)";
        renderer->_skyProgram=CompileProgram(skyVertex,skyFragment);
        const char* waterVertex=R"(#version 300 es
layout(location=0) in vec2 position;
uniform mat4 viewProjection;
uniform mat4 view;
out vec3 uvAndDistance;
out vec4 reflectionPosition;
void main(){
    vec4 world=vec4(position.x,0.0,position.y,1.0);
    vec4 clip=viewProjection*world;
    gl_Position=clip;
    uvAndDistance=vec3(world.xz/500.0,(view*world).z);
    reflectionPosition=vec4((clip.xy+clip.ww)*0.5,clip.zw);
}
)";
        const char* waterFragment=R"(#version 300 es
precision highp float;
in vec3 uvAndDistance;
in vec4 reflectionPosition;
uniform sampler2D diffuse;
uniform sampler2D alphaMap;
uniform sampler2D reflection;
uniform float skyType;
out vec4 outputColor;
void main(){
    vec3 color=mix(0.25,1.0,clamp(skyType/2.0,0.0,1.0))*texture(diffuse,uvAndDistance.xy).rgb;
    vec3 reflected=texture(reflection,reflectionPosition.xy/reflectionPosition.w).rgb;
    float opacity=texture(alphaMap,uvAndDistance.xy).r;
    float horizon=clamp(3000.0/max(abs(uvAndDistance.z),0.001),0.0,1.0);
    outputColor=vec4(mix(reflected,color,opacity*horizon),1.0);
}
)";
        renderer->_waterProgram=CompileProgram(waterVertex,waterFragment);
        const char* footprintVertex=R"(#version 300 es
layout(location=0) in vec2 position;
layout(location=1) in vec2 texcoord;
uniform mat4 viewProjection;
uniform mat4 model;
out vec2 uv;
void main(){gl_Position=viewProjection*model*vec4(position.x,0.0,position.y,1.0);gl_Position.z=0.0;uv=texcoord;}
)";
        const char* footprintFragment=R"(#version 300 es
precision highp float;
in vec2 uv;
uniform sampler2D diffuse;
out vec4 outputColor;
void main(){outputColor=texture(diffuse,uv);}
)";
        renderer->_footprintProgram=CompileProgram(footprintVertex,footprintFragment);
        if (!renderer->_program || !renderer->_objectProgram || !renderer->_skyProgram || !renderer->_waterProgram || !renderer->_footprintProgram) return nullptr;
        glGenVertexArrays(1,&renderer->_vao);
        glGenFramebuffers(1,&renderer->_framebuffer);
        emscripten_log(EM_LOG_CONSOLE,"Shade WebGL2 renderer initialized (original terrain, models, animations and sky)");
        return renderer;
    }
    ~RendererWebGl() noexcept override {
        if (_program) glDeleteProgram(_program);
        if (_objectProgram) glDeleteProgram(_objectProgram);
        if (_skyProgram) glDeleteProgram(_skyProgram);
        if (_waterProgram) glDeleteProgram(_waterProgram);
        if (_footprintProgram)glDeleteProgram(_footprintProgram);
        if (_framebuffer)glDeleteFramebuffers(1,&_framebuffer);
        if (_vao) glDeleteVertexArrays(1,&_vao);
        if (_context > 0) emscripten_webgl_destroy_context(_context);
    }
	[[nodiscard]] void* GetDevice() const noexcept final { return nullptr; }
	void DrawScene(const DrawSceneDesc& desc) noexcept final {
        if ((desc.viewId != RenderPass::Main && desc.viewId != RenderPass::Reflection) || !desc.camera) return;
        if(desc.viewId==RenderPass::Main && desc.drawIsland && Locator::terrainSystem::has_value())DrawFootprints();
        if(desc.viewId==RenderPass::Main && desc.drawWater && Locator::oceanSystem::has_value()) {
            const auto reflectedCamera=desc.camera->Reflect();
            auto reflected=desc;reflected.camera=reflectedCamera.get();reflected.viewId=RenderPass::Reflection;
            reflected.drawWater=false;reflected.drawBoundingBoxes=false;
            reflected.frameBuffer=&Locator::oceanSystem::value().GetReflectionFramebuffer();
            DrawScene(reflected);
        }
        int width=0,height=0;
        if(desc.frameBuffer) {
            if(!BindTarget(*desc.frameBuffer))return;
            width=desc.frameBuffer->resolution.x;height=desc.frameBuffer->resolution.y;
        }else {glBindFramebuffer(GL_FRAMEBUFFER,0);emscripten_get_canvas_element_size("#canvas",&width,&height);}
        if (!_reported) emscripten_log(EM_LOG_CONSOLE,"Upload GL error: 0x%x",glGetError());
        glViewport(0,0,width,height);
        glDepthMask(GL_TRUE);glDisable(GL_SCISSOR_TEST);
        glClearColor(0.45f,0.65f,0.85f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        if (!Locator::terrainSystem::has_value()) return;
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); glDisable(GL_CULL_FACE);
        glBindVertexArray(_vao);
        if(desc.drawSky && Locator::skySystem::has_value())DrawSky(*desc.camera);
        if(desc.drawWater && Locator::oceanSystem::has_value())DrawWater(*desc.camera);
        glUseProgram(_program); glBindVertexArray(_vao);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY,static_cast<GLuint>(Locator::terrainSystem::value().GetAlbedoArray().handle.id));
        glUniform1i(glGetUniformLocation(_program,"materials"),0);
        const auto& island=Locator::terrainSystem::value();
        const auto& textures=Locator::resources::value().GetTextures();
        const auto extent=island.GetExtent();
        const auto extentVector=glm::vec4(extent.minimum,extent.maximum);
        glUniform4fv(glGetUniformLocation(_program,"islandExtent"),1,glm::value_ptr(extentVector));
        const auto view=desc.camera->GetViewMatrix(Camera::Interpolation::Current);
        glUniformMatrix4fv(glGetUniformLocation(_program,"view"),1,GL_FALSE,glm::value_ptr(view));
        const auto skyType=Locator::skySystem::value().GetCurrentSkyType();
        glUniform3f(glGetUniformLocation(_program,"skyAndBump"),skyType,desc.bumpMapStrength,desc.smallBumpMapStrength);
        const auto smallBump=textures.Contains(LandIslandInterface::k_SmallBumpTextureId)?textures.Handle(LandIslandInterface::k_SmallBumpTextureId)->handle.id:0;
        const GLuint terrainTextures[]={GLuint(island.GetBump().handle.id),GLuint(smallBump),GLuint(island.GetFootprintFramebuffer().colorAttachment.id)};
        const char* terrainNames[]={"bumpMap","smallBump","footprint"};
        for(int i=0;i<3;++i){glActiveTexture(GL_TEXTURE1+i);glBindTexture(GL_TEXTURE_2D,terrainTextures[i]);glUniform1i(glGetUniformLocation(_program,terrainNames[i]),i+1);}
        glActiveTexture(GL_TEXTURE0);
        if (!_reported) {
            const auto origin=desc.camera->GetOrigin(); const auto focus=desc.camera->GetFocus();
            emscripten_log(EM_LOG_CONSOLE,"Terrain: %zu blocks, vertex size %zu, camera %.2f %.2f %.2f -> %.2f %.2f %.2f",
                Locator::terrainSystem::value().GetBlocks().size(),sizeof(LandVertex),origin.x,origin.y,origin.z,focus.x,focus.y,focus.z);
            _reported=true;
        }
        const auto matrix=desc.camera->GetViewProjectionMatrix(Camera::Projection::Normal);
        glUniformMatrix4fv(glGetUniformLocation(_program,"viewProjection"),1,GL_FALSE,glm::value_ptr(matrix));
        if(desc.drawIsland) for (const auto& block: Locator::terrainSystem::value().GetBlocks()) {
            const auto& buffer=block.GetMesh().GetVertexBuffer();
            if (!buffer.handle.id) continue;
            glBindBuffer(GL_ARRAY_BUFFER,static_cast<GLuint>(buffer.handle.id));
            glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,buffer.strideBytes,nullptr);
            glEnableVertexAttribArray(1); glVertexAttribPointer(1,4,GL_UNSIGNED_BYTE,GL_TRUE,buffer.strideBytes,reinterpret_cast<void*>(36));
            glEnableVertexAttribArray(2); glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,buffer.strideBytes,reinterpret_cast<void*>(12));
            glEnableVertexAttribArray(3); glVertexAttribIPointer(3,3,GL_UNSIGNED_BYTE,buffer.strideBytes,reinterpret_cast<void*>(24));
            glEnableVertexAttribArray(4); glVertexAttribIPointer(4,3,GL_UNSIGNED_BYTE,buffer.strideBytes,reinterpret_cast<void*>(28));
            glEnableVertexAttribArray(5); glVertexAttribPointer(5,3,GL_UNSIGNED_BYTE,GL_TRUE,buffer.strideBytes,reinterpret_cast<void*>(32));
            glEnableVertexAttribArray(6); glVertexAttribPointer(6,1,GL_FLOAT,GL_FALSE,buffer.strideBytes,reinterpret_cast<void*>(40));
            const auto offset=block.GetMapPosition();
            glUniform2f(glGetUniformLocation(_program,"offset"),offset.x,offset.y);
            glDrawArrays(GL_TRIANGLES,0,buffer.vertexCount);
        }
        if (!_modelsReported) emscripten_log(EM_LOG_CONSOLE,"Terrain GL error: 0x%x",glGetError());
        glUseProgram(_objectProgram);
        glUniform1f(glGetUniformLocation(_objectProgram,"skyType"),skyType);
        glUniform4fv(glGetUniformLocation(_objectProgram,"islandExtent"),1,glm::value_ptr(extentVector));
        glActiveTexture(GL_TEXTURE4);glBindTexture(GL_TEXTURE_2D,island.GetHeightMap().handle.id);
        glUniform1i(glGetUniformLocation(_objectProgram,"heightMap"),4);glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(glGetUniformLocation(_objectProgram,"viewProjection"),1,GL_FALSE,glm::value_ptr(matrix));
        const auto& context=Locator::rendereringSystem::value().GetContext();
        const auto& meshes=Locator::resources::value().GetMeshes();
        if (desc.drawEntities) for(const auto& [key,instances]:context.instancedDrawDescs) {
            if(!meshes.Contains(key.meshId)) continue;
            const auto mesh=meshes.Handle(key.meshId);
            for(uint32_t i=0;i<instances.count;++i) {
                const auto& animation=context.instanceAnimations.at(instances.offset+i);
                const auto time=animation.started<0 ? desc.time+animation.phase : uint32_t(std::max<int64_t>(0,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()-animation.started));
                DrawObject(*mesh,context.instanceUniforms.at(instances.offset+i),animation.clip,time,instances.morphWithTerrain);
            }
        }
        if(desc.drawVegetation) for(const auto& [id,instances]:context.treeInstancedDrawDescs) {
            if(!meshes.Contains(id)) continue;
            const auto mesh=meshes.Handle(id);
            for(uint32_t i=0;i<instances.count;++i)
                DrawObject(*mesh,context.treeInstanceData.at(instances.offset+i).modelMatrix,AnimId::Invalid,0,false,&context.treeInstanceData.at(instances.offset+i).swayParams);
        }
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);
        glBindVertexArray(0);
        if (!_modelsReported) {
            if(meshes.Contains(std::string("hand"))) {
                const auto hand=meshes.Handle(resources::HashIdentifier(std::string("hand")));
                size_t uploadedSkins=0;
                for(const auto& [id,skin]:hand->GetSkins()) if(skin && skin->handle.id)++uploadedSkins;
                emscripten_log(EM_LOG_CONSOLE,"Original god hand: %s, %zu bones, %zu/%zu original skins uploaded",
                    hand->GetDebugName().c_str(),hand->GetBoneMatrices().size(),uploadedSkins,hand->GetSkins().size());
            }
            emscripten_log(EM_LOG_CONSOLE,"Model batches: %zu entities, %zu trees; GL error 0x%x",
                context.instancedDrawDescs.size(),context.treeInstancedDrawDescs.size(),glGetError());
            _modelsReported=true;
        }
    }
	void Frame() noexcept final {glBindFramebuffer(GL_FRAMEBUFFER,0);if(Locator::debugGui::has_value())Locator::debugGui::value().Draw(nullptr,nullptr);}
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
        {
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
        GLuint handle=0; glGenBuffers(1,&handle);
        // WebGL distinguishes index buffers from vertex buffers at first bind.
        // Preserve the draw VAO while creating this element buffer.
        GLint previousVao=0;glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&previousVao);
        glBindVertexArray(0);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,handle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,memory.size(),memory.data(),GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);glBindVertexArray(previousVao);
        return {new (std::nothrow) IndexBuffer {std::move(name),static_cast<uint32_t>(memory.size()/stride),type,{handle}},
                [](IndexBuffer* buffer) { const auto id=static_cast<GLuint>(buffer->handle.id);glDeleteBuffers(1,&id);delete buffer; }};
	}

	[[nodiscard]] Texture2dUniquePtr CreateTexture2d(std::string name, std::span<const uint8_t> memory, glm::u16vec2 resolution,
	                                                 uint16_t layers, TextureFormat format, Wrapping wrapping, Filter filter) noexcept final
	{
		GLuint handle=0;
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        if (format == TextureFormat::BGR5A1 && layers > 1) {
            const size_t count=static_cast<size_t>(resolution.x)*resolution.y*layers;
            if (memory.size() < count*2) return nullptr;
            std::vector<uint8_t> rgba(count*4);
            for (size_t i=0;i<count;++i) {
                const uint16_t pixel=memory[i*2] | (uint16_t(memory[i*2+1])<<8);
                rgba[i*4]=((pixel>>10)&31)*255/31;
                rgba[i*4+1]=((pixel>>5)&31)*255/31;
                rgba[i*4+2]=(pixel&31)*255/31;
                rgba[i*4+3]=255;
            }
            glGenTextures(1,&handle); glBindTexture(GL_TEXTURE_2D_ARRAY,handle);
            glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA8,resolution.x,resolution.y,layers,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        }
        else if(layers == 1 && !memory.empty()) {
            const size_t count=static_cast<size_t>(resolution.x)*resolution.y;
            std::vector<uint8_t> rgba;
            if(format==TextureFormat::BlockCompression1 || format==TextureFormat::BlockCompression2 || format==TextureFormat::BlockCompression3) {
                const size_t needed=((resolution.x+3)/4)*((resolution.y+3)/4)*(format==TextureFormat::BlockCompression1 ? 8 : 16);
                if(memory.size()<needed) return nullptr;
                rgba=detail::DecodeBlockCompressed(format,memory,resolution.x,resolution.y,1);
            } else if(format==TextureFormat::BGRA4 && memory.size()>=count*2) {
                rgba.resize(count*4);
                for(size_t i=0;i<count;++i) {
                    const uint16_t pixel=memory[i*2]|(uint16_t(memory[i*2+1])<<8);
                    rgba[i*4]=((pixel>>8)&15)*17;rgba[i*4+1]=((pixel>>4)&15)*17;
                    rgba[i*4+2]=(pixel&15)*17;rgba[i*4+3]=(pixel>>12)*17;
                }
            } else if((format==TextureFormat::R8 || format==TextureFormat::A8) && memory.size()>=count) {
                rgba.resize(count*4);
                for(size_t i=0;i<count;++i){rgba[i*4]=rgba[i*4+1]=rgba[i*4+2]=memory[i];rgba[i*4+3]=255;}
            } else if((format==TextureFormat::RGBA8 || format==TextureFormat::BGRA8) && memory.size()>=count*4) {
                rgba.assign(memory.begin(),memory.begin()+count*4);
                if(format==TextureFormat::BGRA8)for(size_t i=0;i<count;++i)std::swap(rgba[i*4],rgba[i*4+2]);
            }
            if(!rgba.empty()) {
                glGenTextures(1,&handle);glBindTexture(GL_TEXTURE_2D,handle);
                glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,resolution.x,resolution.y,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
                const GLint sampling=filter==Filter::Linear ? GL_LINEAR : GL_NEAREST;
                const GLint address=wrapping==Wrapping::Repeat ? GL_REPEAT : wrapping==Wrapping::MirroredRepeat ? GL_MIRRORED_REPEAT : GL_CLAMP_TO_EDGE;
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,sampling);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,sampling);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,address);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,address);
            }
        }
        return {new (std::nothrow) Texture2d {std::move(name), {handle}, {}, resolution, layers, format},
                [](Texture2d* texture) { const auto id=static_cast<GLuint>(texture->handle.id); if(id)glDeleteTextures(1,&id); delete texture; }};
	}

	[[nodiscard]] std::unique_ptr<FrameBuffer> CreateFrameBuffer(std::string name, glm::u16vec2 resolution,
	                                                             TextureFormat colorFormat,
	                                                             std::optional<TextureFormat> depthStencilFormat) noexcept final
	{
        GLuint color=0,depth=0;
        glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,resolution.x,resolution.y,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        if(depthStencilFormat) {
            glGenTextures(1,&depth);glBindTexture(GL_TEXTURE_2D,depth);
            glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH24_STENCIL8,resolution.x,resolution.y,0,GL_DEPTH_STENCIL,GL_UNSIGNED_INT_24_8,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        }
        auto result=std::make_unique<FrameBuffer>(std::move(name),resolution,colorFormat,depthStencilFormat,TextureHandle{color},SamplerHandle{},TextureHandle{depth},[color,depth]{glDeleteTextures(1,&color);if(depth)glDeleteTextures(1,&depth);});
        BindTarget(*result);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glBindFramebuffer(GL_FRAMEBUFFER,0);
        return result;
	}

private:
    void DrawFootprints() const noexcept {
        const auto& island=Locator::terrainSystem::value();
        const auto& target=island.GetFootprintFramebuffer();
        if(!BindTarget(target))return;
        glViewport(0,0,target.resolution.x,target.resolution.y);glDisable(GL_SCISSOR_TEST);
        glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(_vao);glUseProgram(_footprintProgram);glActiveTexture(GL_TEXTURE0);
        const auto matrix=island.GetOrthoProj()*island.GetOrthoView();
        glUniformMatrix4fv(glGetUniformLocation(_footprintProgram,"viewProjection"),1,GL_FALSE,glm::value_ptr(matrix));
        glUniform1i(glGetUniformLocation(_footprintProgram,"diffuse"),0);
        const auto& context=Locator::rendereringSystem::value().GetContext();const auto& meshes=Locator::resources::value().GetMeshes();
        unsigned count=0;
        auto draw=[&](entt::id_type id,const auto& instances,bool tree){
            if(!meshes.Contains(id))return;const auto mesh=meshes.Handle(id);
            if(!mesh->ContainsLandscapeFeature()||mesh->GetFootprints().empty())return;
            const auto& decal=mesh->GetFootprints().front();if(!decal.texture||!decal.texture->handle.id||!decal.mesh)return;
            const auto& vertices=decal.mesh->GetVertexBuffer();
            glBindTexture(GL_TEXTURE_2D,decal.texture->handle.id);glBindBuffer(GL_ARRAY_BUFFER,vertices.handle.id);
            for(GLuint i=2;i<7;++i)glDisableVertexAttribArray(i);
            glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,vertices.strideBytes,nullptr);
            glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,vertices.strideBytes,reinterpret_cast<void*>(8));
            for(uint32_t i=0;i<instances.count;++i){
                const auto& model=tree?context.treeInstanceData.at(instances.offset+i).modelMatrix:context.instanceUniforms.at(instances.offset+i);
                glUniformMatrix4fv(glGetUniformLocation(_footprintProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES,0,vertices.vertexCount);++count;
            }
        };
        for(const auto& [key,instances]:context.instancedDrawDescs)draw(key.meshId,instances,false);
        for(const auto& [id,instances]:context.treeInstancedDrawDescs)draw(id,instances,true);
        glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);
        if(!_footprintsReported){emscripten_log(EM_LOG_CONSOLE,"Original terrain footprints: %u draws; target %ux%u; GL error 0x%x",count,target.resolution.x,target.resolution.y,glGetError());_footprintsReported=true;}
    }
    bool BindTarget(const FrameBuffer& target) const noexcept {
        glBindFramebuffer(GL_FRAMEBUFFER,_framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target.colorAttachment.id,0);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,target.depthStencilAttachment.id,0);
        const auto status=glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(status!=GL_FRAMEBUFFER_COMPLETE){emscripten_log(EM_LOG_ERROR,"Framebuffer %s incomplete: 0x%x",target.name.c_str(),status);return false;}
        return true;
    }
    void DrawWater(const Camera& camera) const noexcept {
        const auto& ocean=Locator::oceanSystem::value();
        const auto& textures=Locator::resources::value().GetTextures();
        if(!textures.Contains(ocean.GetDiffuseTexture())||!textures.Contains(ocean.GetAlphaTexture()))return;
        const auto diffuse=textures.Handle(ocean.GetDiffuseTexture()),alpha=textures.Handle(ocean.GetAlphaTexture());
        if(!diffuse->handle.id||!alpha->handle.id)return;
        glUseProgram(_waterProgram);glDisable(GL_BLEND);glDepthMask(GL_TRUE);
        const auto projection=camera.GetViewProjectionMatrix(Camera::Projection::Normal),view=camera.GetViewMatrix(Camera::Interpolation::Current);
        glUniformMatrix4fv(glGetUniformLocation(_waterProgram,"viewProjection"),1,GL_FALSE,glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(_waterProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniform1f(glGetUniformLocation(_waterProgram,"skyType"),Locator::skySystem::value().GetCurrentSkyType());
        const GLuint ids[]={GLuint(diffuse->handle.id),GLuint(alpha->handle.id),GLuint(ocean.GetReflectionFramebuffer().colorAttachment.id)};
        const char* names[]={"diffuse","alphaMap","reflection"};
        for(int i=0;i<3;++i){glActiveTexture(GL_TEXTURE0+i);glBindTexture(GL_TEXTURE_2D,ids[i]);glUniform1i(glGetUniformLocation(_waterProgram,names[i]),i);}
        const auto& vertices=ocean.GetMesh().GetVertexBuffer();const auto& indices=ocean.GetMesh().GetIndexBuffer();
        glBindBuffer(GL_ARRAY_BUFFER,vertices.handle.id);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices.handle.id);
        for(GLuint i=1;i<7;++i)glDisableVertexAttribArray(i);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,vertices.strideBytes,nullptr);
        glDrawElements(GL_TRIANGLES,indices.count,GL_UNSIGNED_SHORT,nullptr);
        glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,0);glActiveTexture(GL_TEXTURE0);
        if(!_waterReported){emscripten_log(EM_LOG_CONSOLE,"Original water and reflected scene: %u indices; GL error 0x%x",indices.count,glGetError());_waterReported=true;}
    }
    void DrawSky(const Camera& camera) const noexcept {
        const auto& sky=Locator::skySystem::value();
        if(!sky.GetTexture().handle.id || sky.GetMesh().GetSubMeshes().empty())return;
        const auto& geometry=sky.GetMesh().GetSubMeshes()[0]->GetMesh();
        const auto& vertices=geometry.GetVertexBuffer();
        const auto matrix=camera.GetViewProjectionMatrix(Camera::Projection::Normal);
        glUseProgram(_skyProgram);glDepthMask(GL_FALSE);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D_ARRAY,sky.GetTexture().handle.id);
        glUniform1i(glGetUniformLocation(_skyProgram,"skies"),0);
        glUniform2f(glGetUniformLocation(_skyProgram,"skyData"),sky.GetCurrentSkyType(),Locator::config::value().skyAlignment+1.0f);
        glUniformMatrix4fv(glGetUniformLocation(_skyProgram,"viewProjection"),1,GL_FALSE,glm::value_ptr(matrix));
        glBindBuffer(GL_ARRAY_BUFFER,vertices.handle.id);
        for(GLuint i=2;i<7;++i)glDisableVertexAttribArray(i);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,vertices.strideBytes,nullptr);
        glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,vertices.strideBytes,reinterpret_cast<void*>(12));
        if(geometry.IsIndexed()) {
            const auto& indices=geometry.GetIndexBuffer();glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices.handle.id);
            glDrawElements(GL_TRIANGLES,indices.count,indices.type==IndexBuffer::Type::Uint16?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,nullptr);
        }else glDrawArrays(GL_TRIANGLES,0,vertices.vertexCount);
        glEnable(GL_DEPTH_TEST);glDepthMask(GL_TRUE);
        if(!_skyReported){emscripten_log(EM_LOG_CONSOLE,"Original sky: %u texture layers, %u vertices; GL error 0x%x",sky.GetTexture().numLayers,vertices.vertexCount,glGetError());_skyReported=true;}
    }
    static GLuint CompileProgram(const char* vertex,const char* fragment) noexcept {
        GLuint program=glCreateProgram();
        const char* sources[2]={vertex,fragment};
        const GLenum types[2]={GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
        for(int i=0;i<2;++i) {
            GLuint shader=glCreateShader(types[i]);
            glShaderSource(shader,1,&sources[i],nullptr);glCompileShader(shader);
            GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
            if(!ok) { char log[2048];glGetShaderInfoLog(shader,sizeof(log),nullptr,log);
                emscripten_log(EM_LOG_ERROR,"WebGL shader: %s",log);glDeleteShader(shader);glDeleteProgram(program);return 0; }
            glAttachShader(program,shader);glDeleteShader(shader);
        }
        glLinkProgram(program);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
        if(!ok) { char log[2048];glGetProgramInfoLog(program,sizeof(log),nullptr,log);
            emscripten_log(EM_LOG_ERROR,"WebGL program: %s",log);glDeleteProgram(program);return 0; }
        return program;
    }
    void DrawObject(const L3DMesh& mesh,const glm::mat4& model, AnimId clip=AnimId::Invalid, uint32_t time=0,bool morph=false,const glm::vec4* sway=nullptr) const noexcept {
        glUniform1i(glGetUniformLocation(_objectProgram,"morphWithTerrain"),morph);
        glUniform1i(glGetUniformLocation(_objectProgram,"vegetation"),sway!=nullptr);
        if(sway)glUniform4fv(glGetUniformLocation(_objectProgram,"sway"),1,glm::value_ptr(*sway));
        const auto* palette=&mesh.GetBoneMatrices();
        std::vector<glm::mat4> posed;
        auto& animations=Locator::resources::value().GetAnimations();
        const auto clipId=resources::HashIdentifier(clip);
        if (mesh.IsBoned() && clip >= AnimId::BatEraticglide && animations.Contains(clipId)) {
            const auto clipResource=animations.Handle(clipId);
            posed=clipResource->GetBoneMatrices(time);
            if (animation::ComposeHierarchy(posed,mesh.GetBoneParents())) {
                palette=&posed;
                if (!_animationReported) {
                    emscripten_log(EM_LOG_CONSOLE,"Original animation playback: %s, %zu bones, %u ms loop",clipResource->GetName().c_str(),posed.size(),clipResource->GetDuration());
                    _animationReported=true;
                }
            }
        }
        const auto& bones=*palette;
        const auto boneCount=mesh.IsBoned() ? bones.size() : 0;
        if(boneCount>64) return;
        glUniformMatrix4fv(glGetUniformLocation(_objectProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(_objectProgram,"boneCount"),boneCount);
        if(boneCount) glUniformMatrix4fv(glGetUniformLocation(_objectProgram,"bones[0]"),boneCount,GL_FALSE,glm::value_ptr(bones[0]));
        const auto& textures=Locator::resources::value().GetTextures();
        for(const auto& sub:mesh.GetSubMeshes()) {
            const auto flags=sub->GetFlags();
            if(flags.isPhysics || flags.status!=0 || (flags.lodMask&1)==0) continue;
            const auto& geometry=sub->GetMesh();
            const auto& vertices=geometry.GetVertexBuffer();const auto& indices=geometry.GetIndexBuffer();
            glBindBuffer(GL_ARRAY_BUFFER,vertices.handle.id);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,indices.handle.id);
            for(GLuint i=4;i<7;++i)glDisableVertexAttribArray(i);
            glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,vertices.strideBytes,nullptr);
            glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,vertices.strideBytes,reinterpret_cast<void*>(12));
            glEnableVertexAttribArray(2);glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,vertices.strideBytes,reinterpret_cast<void*>(20));
            glEnableVertexAttribArray(3);glVertexAttribIPointer(3,2,GL_SHORT,vertices.strideBytes,reinterpret_cast<void*>(32));
            for(const auto& primitive:sub->GetPrimitives()) {
                const Texture2d* texture=nullptr;
                if(auto found=mesh.GetSkins().find(primitive.skinID);found!=mesh.GetSkins().end())texture=found->second.get();
                else if(textures.Contains(primitive.skinID))texture=textures.Handle(primitive.skinID).operator->();
                glBindTexture(GL_TEXTURE_2D,texture ? texture->handle.id : 0);
                glUniform1i(glGetUniformLocation(_objectProgram,"textured"),texture && texture->handle.id);
                glUniform1i(glGetUniformLocation(_objectProgram,"diffuse"),0);
                glUniform1f(glGetUniformLocation(_objectProgram,"alphaCutoff"),primitive.thresholdAlpha ? primitive.alphaCutoutThreshold : 0.01f);
                glDepthMask(primitive.depthWrite ? GL_TRUE : GL_FALSE);
                if(primitive.blend!=L3DSubMesh::Primitive::BlendMode::Disabled) {
                    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,primitive.blend==L3DSubMesh::Primitive::BlendMode::Additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);
                }else glDisable(GL_BLEND);
                glDrawElements(GL_TRIANGLES,primitive.indicesCount,GL_UNSIGNED_SHORT,reinterpret_cast<void*>(primitive.indicesOffset*sizeof(uint16_t)));
                if(!_modelsReported && !_objectErrorReported) {
                    const auto error=glGetError();
                    if(error) { emscripten_log(EM_LOG_ERROR,"Model %s: GL error 0x%x, stride %u, vertices %u, indices %u",mesh.GetDebugName().c_str(),error,vertices.strideBytes,vertices.vertexCount,indices.count);_objectErrorReported=true; }
                }
            }
        }
    }
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE _context {};
    GLuint _program {}, _objectProgram {}, _skyProgram {}, _waterProgram {}, _footprintProgram {}, _vao {}, _framebuffer {};
	bool _reported {}, _modelsReported {};
    mutable bool _objectErrorReported {}, _animationReported {}, _skyReported {}, _waterReported {}, _footprintsReported {};
	bool _debug {};
	bool _profile {};
};

} // namespace openblack::graphics
