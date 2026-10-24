#pragma once
#include <cstdint>
#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>
#include "3D/DroughtGeometry.h"
#include <glm/mat4x4.hpp>
namespace openblack::browser {
inline float droughtRain=0,droughtDryness=0;
inline std::vector<GroundVertex> droughtGround;
inline unsigned droughtGroundRevision=0;
inline uint32_t droughtCreatureMesh=0;
void RestoreDroughtCounters(int deliveries);
int SwapDroughtGod(int selection);
void AnimateDroughtCreature(int phase,float progress,float time,int paused,int deliveries);
const std::vector<glm::mat4>& SampleDroughtCreature();
void UpdateDroughtWorld(int discovered,int channel,float progress,float time);
int DroughtRoutes();
void UpdateBrowserHeldObjects();
bool PlaceDroughtVillager(entt::entity entity,glm::vec3& position);
int RunDrought(int phase,float progress,int deliveries,int focus,int reset);
void RunDroughtWorker(int index,int job,float progress);
}
