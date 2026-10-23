#pragma once
#include <algorithm>
#include <cmath>
#include <span>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
namespace openblack::browser {
struct GroundVertex { glm::vec3 position; glm::vec4 color; };
// Ground-following ribbons and pools: visual waterworks, not a fluid simulation.
template<class Height>
std::vector<GroundVertex> BuildWaterworks(std::span<const glm::vec3> path,glm::vec3 well,bool discovered,bool complete,float progress,float time,Height height){
    std::vector<GroundVertex> out;
    auto vertex=[&](glm::vec3 p,glm::vec4 color,float lift){p.y=height(p.x,p.z)+lift;return GroundVertex{p,color};};
    auto triangle=[&](glm::vec3 a,glm::vec3 b,glm::vec3 c,glm::vec4 color,float lift){out.push_back(vertex(a,color,lift));out.push_back(vertex(b,color,lift));out.push_back(vertex(c,color,lift));};
    auto pool=[&](glm::vec3 center,float radius,glm::vec4 color,float lift){for(int i=0;i<24;++i){float a=i*6.2831853f/24,b=(i+1)*6.2831853f/24;triangle(center,center+glm::vec3(std::cos(a)*radius,0,std::sin(a)*radius),center+glm::vec3(std::cos(b)*radius,0,std::sin(b)*radius),color,lift);}};
    const auto blue=glm::vec4(.12f,.48f+.035f*std::sin(time*2),.57f,.9f);
    pool(well,3.4f,{.25f,.22f,.13f,1},.08f);pool(well,2.4f,blue,.13f);
    if(path.empty())return out;
    if(discovered){pool(path.front(),4.3f,{.27f,.25f,.15f,1},.08f);pool(path.front(),3.3f,blue,.14f);}
    if(!discovered||path.size()<2)return out;
    progress=complete?1:std::clamp(progress,0.f,1.f);
    const float length=progress*(path.size()-1);
    for(size_t i=0;i+1<path.size()&&i<length;++i){
        auto a=path[i],b=a+(path[i+1]-a)*std::min(1.f,length-float(i));
        auto delta=b-a;float len=std::hypot(delta.x,delta.z);if(len<.001f)continue;
        glm::vec3 side(-delta.z/len,0,delta.x/len);
        auto ribbon=[&](float width,glm::vec4 color,float lift){auto s=side*width;triangle(a-s,a+s,b+s,color,lift);triangle(a-s,b+s,b-s,color,lift);};
        ribbon(1.9f,{.32f,.25f,.13f,1},.08f);
        ribbon(1.3f,complete?blue:glm::vec4(.18f,.14f,.08f,1),.14f);
        if(complete){const float p=std::fmod(time*.23f+float(i)*.19f,1.f);auto s=side*.65f;auto c=a+(b-a)*p;triangle(c-s,c+s,c+delta*.13f,{.50f,.72f,.68f,.45f},.18f);}
    }
    return out;
}
}
