#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <span>
#include <vector>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include "MorphFile.h"
#include "AnimationSampling.h"
namespace openblack::animation {
struct CreatureChannels {std::vector<glm::quat> rotations;std::vector<glm::vec3> translations;};
inline bool ReadCreatureFrame(const morph::Animation& clip,size_t frame,CreatureChannels& result){
    if(frame>=clip.keyframes.size())return false;
    const auto& key=clip.keyframes[frame];
    if(key.eulerAngles.size()!=clip.rotatedJointIndices.size()||key.translations.size()!=clip.translatedJointIndices.size())return false;
    for(size_t i=0;i<key.eulerAngles.size();++i){auto joint=clip.rotatedJointIndices[i];if(joint>=result.rotations.size())return false;const auto& v=key.eulerAngles[i];
        if(!std::isfinite(v[0])||!std::isfinite(v[1])||!std::isfinite(v[2]))return false;
        result.rotations[joint]=glm::quat_cast(glm::transpose(glm::eulerAngleXYZ(v[0],v[1],v[2])));}
    for(size_t i=0;i<key.translations.size();++i){auto joint=clip.translatedJointIndices[i];if(joint>=result.translations.size())return false;const auto& v=key.translations[i];
        if(!std::isfinite(v[0])||!std::isfinite(v[1])||!std::isfinite(v[2]))return false;result.translations[joint]={v[0],v[1],v[2]};}
    return true;
}
inline std::vector<glm::mat4> SampleCreature(const morph::Animation& clip,std::span<const glm::mat4> bindLocal,std::span<const uint32_t> parents,const CreatureChannels& base,float milliseconds,bool loop=true,bool inPlace=false){
    if(clip.keyframes.empty()||bindLocal.size()!=parents.size()||base.rotations.size()!=parents.size()||base.translations.size()!=parents.size()||clip.header.meshBoneCount!=parents.size()||!std::isfinite(milliseconds))return {};
    const auto duration=std::max(1u,clip.header.unknown0x0);
    float tick=std::max(0.f,milliseconds);tick=loop?std::fmod(tick,float(duration)):std::min(tick,float(duration));
    float frame=tick/duration*(loop?clip.keyframes.size():clip.keyframes.size()-1);
    size_t a=std::min(size_t(frame),clip.keyframes.size()-1),b=loop?(a+1)%clip.keyframes.size():std::min(a+1,clip.keyframes.size()-1);
    auto before=base,after=base;if(!ReadCreatureFrame(clip,a,before)||!ReadCreatureFrame(clip,b,after))return {};
    std::vector<glm::mat4> out(bindLocal.size());const float factor=std::clamp(frame-float(a),0.f,1.f);
    for(size_t i=0;i<out.size();++i){auto position=glm::mix(before.translations[i],after.translations[i],factor);
        if(inPlace&&parents[i]==std::numeric_limits<uint32_t>::max()){position.x=base.translations[i].x;position.z=base.translations[i].z;}
        // Original CBN rotations use row-vector axes; transpose into GLM column-vector axes.
        auto rotation=glm::mat4(glm::mat3(bindLocal[i]))*glm::mat4_cast(glm::slerp(before.rotations[i],after.rotations[i],factor));
        out[i]=glm::translate(glm::mat4(1),position)*rotation;
    }
    if(!ComposeHierarchy(out,parents))return {};return out;
}
}
