#include "3D/AnimationSampling.h"
#include "ANMFile.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
using namespace openblack;
int main() {
    std::vector<L3DAnim::Frame> frames={{0,{glm::mat4(1)}},{500,{glm::translate(glm::mat4(1),glm::vec3(10,0,0))}}};
    assert(animation::SampleLoop(frames,1000,0)[0][3].x==0);
    assert(animation::SampleLoop(frames,1000,250)[0][3].x==5);
    assert(animation::SampleLoop(frames,1000,750)[0][3].x==5);
    assert(animation::SampleLoop(frames,1000,1000)[0][3].x==0);
    std::vector<glm::mat4> bones={glm::translate(glm::mat4(1),glm::vec3(2,0,0)),glm::translate(glm::mat4(1),glm::vec3(0,3,0))};
    const std::array<uint32_t,2> parents={UINT32_MAX,0};
    assert(animation::ComposeHierarchy(bones,parents));assert(bones[1][3].x==2 && bones[1][3].y==3);
    const std::array<uint32_t,2> cyclic={1,0};assert(!animation::ComposeHierarchy(bones,cyclic));
    std::vector<uint8_t> bytes(220);
    anm::ANMHeader header{};header.unknown0x20=1000;header.animationDuration=220;header.frameCount=2;header.framesBase=84;
    std::memcpy(bytes.data(),&header,sizeof(header));
    auto word=[&](size_t offset,uint32_t value){std::memcpy(bytes.data()+offset,&value,4);};
    word(84,92);word(88,156);word(92,96);word(96,100);word(100,1);word(104,108);
    word(156,160);word(160,164);word(164,1);word(168,172);
    anm::ANMBone matrix{};matrix.matrix={1,0,0,0,1,0,0,0,1,0,0,0};
    std::memcpy(bytes.data()+108,&matrix,sizeof(matrix));matrix.matrix[9]=10;std::memcpy(bytes.data()+172,&matrix,sizeof(matrix));
    anm::ANMFile parsed;assert(parsed.Open(bytes)==anm::ANMResult::Success);
    assert(parsed.GetDuration()==1000 && parsed.GetKeyframes()[0].time==0 && parsed.GetKeyframes()[1].time==500);
    assert(parsed.GetKeyframes()[1].bones[0].matrix[9]==10);
    word(168,10000);anm::ANMFile invalid;assert(invalid.Open(bytes)==anm::ANMResult::ErrFileTooSmall);
    std::cout<<"Animation timing, interpolation, loop wrap, hierarchy and invalid offsets passed\n";
}
