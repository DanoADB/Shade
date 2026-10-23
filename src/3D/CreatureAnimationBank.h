#pragma once
#include <cstring>
#include <map>
#include <string>
#include "CreatureAnimationSampling.h"
namespace openblack::animation {
class CreatureAnimationBank {
    std::map<std::string,morph::Animation> clips;
    std::vector<glm::mat4> localBind;
    std::vector<uint32_t> parents;
    CreatureChannels base;
public:
    bool Load(const std::vector<uint8_t>& block,const std::filesystem::path& specs,std::span<const glm::mat4> bind,std::span<const uint32_t> hierarchy){
        clips.clear();if(bind.empty()||bind.size()!=hierarchy.size())return false;
        morph::MorphFile morph;if(morph.Open(block,specs)!=morph::MorphResult::Success)return false;
        parents.assign(hierarchy.begin(),hierarchy.end());localBind.assign(bind.begin(),bind.end());
        for(size_t i=0;i<bind.size();++i)if(parents[i]!=UINT32_MAX){if(parents[i]>=i)return false;localBind[i]=glm::inverse(bind[parents[i]])*bind[i];}
        size_t slot=0,loaded=0;
        for(const auto& set:morph.GetAnimationSpecs().animationSets)for(const auto& desc:set.animations){
            const size_t at=sizeof(morph::MorphHeader)+slot++*sizeof(uint32_t);if(at+4>block.size())return false;
            uint32_t offset;std::memcpy(&offset,block.data()+at,4);if(!offset)continue;
            if(loaded>=morph.GetBaseAnimationSet().size())return false;
            const auto& clip=morph.GetBaseAnimationSet()[loaded++];
            if(clip.header.meshBoneCount!=bind.size())return false;
            // Some names occur in multiple categories; use the first full-body motion.
            clips.try_emplace(desc.name,clip);
        }
        if(!clips.contains("stand")||!clips.contains("walk"))return false;
        base.rotations.assign(bind.size(),glm::quat(1,0,0,0));base.translations.resize(bind.size());
        for(size_t i=0;i<bind.size();++i)base.translations[i]=glm::vec3(localBind[i][3]);
        return ReadCreatureFrame(clips.at("stand"),0,base);
    }
    size_t Count()const{return clips.size();}
    unsigned Duration(const std::string& name)const{auto it=clips.find(name);return it==clips.end()?1:std::max(1u,it->second.header.unknown0x0);}
    float TravelDistance(const std::string& name)const{auto it=clips.find(name);if(it==clips.end())return 0;return std::hypot(it->second.header.unknown0x14,it->second.header.unknown0x18);}
    bool Contains(const std::string& name)const{return clips.contains(name);}
    std::vector<glm::mat4> Sample(const std::string& name,float time,bool loop=true)const{
        auto it=clips.find(name);if(it==clips.end())it=clips.find("stand");if(it==clips.end())return {};
        return SampleCreature(it->second,localBind,parents,base,time,loop,name=="walk"||name=="run");
    }
};
}
