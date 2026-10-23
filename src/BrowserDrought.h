#pragma once
namespace openblack::browser {
inline float droughtRain=0,droughtDryness=0;
int RunDrought(int phase,float progress,int deliveries,int focus,int reset);
void RunDroughtWorker(int index,int job,float progress);
}
