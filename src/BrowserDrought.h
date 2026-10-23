#pragma once
#include "3D/DroughtGeometry.h"
namespace openblack::browser {
inline float droughtRain=0,droughtDryness=0;
inline std::vector<GroundVertex> droughtGround;
inline unsigned droughtGroundRevision=0;
void UpdateDroughtWorld(int discovered,int channel,float progress,float time);
int RunDrought(int phase,float progress,int deliveries,int focus,int reset);
void RunDroughtWorker(int index,int job,float progress);
}
