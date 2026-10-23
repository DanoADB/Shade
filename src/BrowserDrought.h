#pragma once
#include <cstdint>
#include "3D/DroughtGeometry.h"
#include <glm/mat4x4.hpp>
namespace openblack::browser {
inline float droughtRain=0,droughtDryness=0;
inline std::vector<GroundVertex> droughtGround;
inline unsigned droughtGroundRevision=0;
inline uint32_t droughtCreatureMesh=0;
int SwapDroughtGod(int selection);
void AnimateDroughtCreature(int phase,float progress,float time,int paused,int deliveries);
const std::vector<glm::mat4>& SampleDroughtCreature();
void UpdateDroughtWorld(int discovered,int channel,float progress,float time);
int RunDrought(int phase,float progress,int deliveries,int focus,int reset);
void RunDroughtWorker(int index,int job,float progress);
}
