#include "3D/CreatureAnimationSampling.h"
#include <cassert>
#include <iostream>
int main(){using namespace openblack;using namespace animation;
 morph::Animation clip{};clip.header.unknown0x0=1000;clip.header.meshBoneCount=2;clip.rotatedJointIndices={0};clip.translatedJointIndices={0};clip.keyframes.resize(2);
 clip.keyframes[0].eulerAngles={{{0,0,0}}};clip.keyframes[1].eulerAngles={{{0,0,3.14159265f}}};clip.keyframes[0].translations={{{0,0,0}}};clip.keyframes[1].translations={{{10,0,0}}};
 CreatureChannels base{{glm::quat(1,0,0,0),glm::quat(1,0,0,0)},{{0,0,0},{0,2,0}}};
 std::vector<glm::mat4> bind(2,glm::mat4(1));std::vector<uint32_t> parents={UINT32_MAX,0};
 bind[1][3].y=2;
 bind[0]=glm::rotate(glm::mat4(1),1.f,glm::vec3(0,1,0));
 auto start=SampleCreature(clip,bind,parents,base,0);auto mid=SampleCreature(clip,bind,parents,base,250);auto end=SampleCreature(clip,bind,parents,base,1000);
 assert(std::abs(start[0][0].x-bind[0][0].x)<.001f); // Zero motion retains joint bind axes.
 assert(start.size()==2&&mid.size()==2&&end.size()==2);assert(std::abs(mid[0][3].x-5)<.001f);assert(glm::length(glm::vec3(mid[1][3])-glm::vec3(mid[0][3]))>1.9f);
 assert(glm::length(glm::vec3(start[0][3])-glm::vec3(end[0][3]))<.001f);
 auto pin=SampleCreature(clip,bind,parents,base,250,true,true);assert(std::abs(pin[0][3].x)<.001f);
 auto held=SampleCreature(clip,bind,parents,base,5000,false);assert(std::abs(held[0][3].x-10)<.001f);
 clip.rotatedJointIndices[0]=99;assert(SampleCreature(clip,bind,parents,base,0).empty());
 std::cout<<"Creature sparse channels, quaternion interpolation, hierarchy, looping, one-shot and root-motion removal passed\n";
}
