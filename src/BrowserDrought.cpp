#ifdef __EMSCRIPTEN__
#include <algorithm>
#include <cmath>
#include <chrono>
#include <vector>
#include <emscripten/emscripten.h>
#include "Locator.h"
#include "Camera/Camera.h"
#include "ECS/Registry.h"
#include "Resources/ResourcesInterface.h"
#include "BrowserDrought.h"
// Scene bridge for Shade's invisible-player drought slice. Rules live in drought-model.js.

#include "3D/CreatureBody.h"
#include "3D/AllMeshes.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/PlayerGiftReaction.h"
#include "ECS/Components/ScenarioActivity.h"
#include "EngineConfig.h"
#include <glm/gtx/euler_angles.hpp>
namespace openblack::browser {
class DroughtScene {
    entt::entity creature{entt::null},water{entt::null};
    glm::vec3 home{},well{},spring{};float scale{1},floorOffset{};int lastDelivery{};std::vector<entt::entity> workers;std::vector<glm::vec3> workerHomes;
    glm::vec3 Ground(glm::vec3 p)const {p.y=Locator::terrainSystem::value().GetHeightAt({p.x,p.z});return p;}
    glm::vec3 Clearing(glm::vec3 desired)const {
        glm::vec3 best=Ground(desired);float bestScore=-1e10f;
        for(int x=-24;x<=24;x+=4)for(int z=-24;z<=24;z+=4){
            auto p=Ground(desired+glm::vec3(x,0,z));float clearance=20;
            Locator::entitiesRegistry::value().Each<const ecs::components::Fixed>([&](const auto& f){clearance=std::min(clearance,glm::distance(glm::vec2(p.x,p.z),f.boundingCenter)-f.boundingRadius);});
            const auto score=std::min(clearance,10.f)*8-std::sqrt(float(x*x+z*z));if(score>bestScore){best=p;bestScore=score;}
        }
        return best;
    }
public:
    void Workers(int index,int job,float progress){
        using namespace ecs::components;
        if(index<0||size_t(index)>=workers.size())return;
        auto& registry=Locator::entitiesRegistry::value();auto e=workers[index];if(!registry.Valid(e))return;
        auto destination=job==0?well:job==1?home+glm::vec3(12,0,8):spring;
        const auto t=std::clamp(progress,0.f,1.f);const auto travel=t<.4f?t/.4f:t>.6f?(1-t)/.4f:1;
        auto& transform=registry.Get<Transform>(e);auto next=Ground(glm::mix(workerHomes[index],destination,travel));auto delta=next-transform.position;delta.y=0;
        if(glm::length(delta)>.01f)transform.rotation=glm::mat3(glm::eulerAngleY(std::atan2(delta.x,delta.z)));
        transform.position=next;registry.AssignOrReplace<ScenarioActivity>(e,glm::length(delta)>.01f);registry.SetDirty();
    }
    int Run(int phase,float progress,int deliveries,int focus,int reset) {
        using namespace ecs::components;
        if(!Locator::entitiesRegistry::has_value()||!Locator::terrainSystem::has_value())return 0;
        auto& registry=Locator::entitiesRegistry::value();auto& meshes=Locator::resources::value().GetMeshes();
        if(reset){if(registry.Valid(creature))registry.Destroy(creature);if(registry.Valid(water))registry.Destroy(water);for(size_t i=0;i<workers.size();++i)if(registry.Valid(workers[i])){registry.Get<Transform>(workers[i]).position=workerHomes[i];registry.Remove<ScenarioActivity>(workers[i]);}
            workers.clear();workerHomes.clear();creature=water=entt::null;lastDelivery=0;}
        if(creature==entt::null){
            const auto id=openblack::creature::GetIdFromType(CreatureType::Cow,openblack::creature::CreatureBody::Appearance::Base);
            if(!meshes.Contains(id))return -1;
            auto mesh=meshes.Handle(id);auto box=mesh->GetBoundingBox();scale=11/std::max(.1f,box.Size().y);floorOffset=-box.minima.y*scale;
            home=Clearing({1920,0,2700});well=Clearing(home+glm::vec3(-28,0,-22));spring=Clearing(home+glm::vec3(32,0,15));
            creature=registry.Create();registry.Assign<Mesh>(creature,id,int8_t(-1),int8_t(-1));registry.Assign<Transform>(creature,home+glm::vec3(0,floorOffset,0),glm::mat3(1),glm::vec3(scale));
            water=registry.Create();registry.Assign<Mesh>(water,resources::HashIdentifier(MeshId::U_Bucket),int8_t(0),int8_t(-1));registry.Assign<Transform>(water,home,glm::mat3(1),glm::vec3(0));
            std::vector<std::pair<float,entt::entity>> candidates;
            registry.Each<const Villager,const Transform>([&](entt::entity e,const Villager&,const Transform& t){
                bool outside=true;registry.Each<const Fixed>([&](const auto& f){if(glm::distance(glm::vec2(t.position.x,t.position.z),f.boundingCenter)<f.boundingRadius+1)outside=false;});
                if(outside)candidates.emplace_back(glm::distance(t.position,home),e);
            });
            std::sort(candidates.begin(),candidates.end());
            for(size_t i=0;i<std::min(size_t(3),candidates.size());++i){workers.push_back(candidates[i].second);workerHomes.push_back(registry.Get<Transform>(workers.back()).position);}
            emscripten_log(EM_LOG_CONSOLE,"Drought workers: %zu original villagers with autonomous jobs",workers.size());
            Locator::config::value().hidePlayerHand=true;focus=1;
            emscripten_log(EM_LOG_CONSOLE,"Drought creature: original cow %s, %zu bones, scale %.3f",mesh->GetDebugName().c_str(),mesh->GetBoneMatrices().size(),scale);
        }
        if(!registry.Valid(creature))return -2;
        progress=std::clamp(progress,0.f,1.f);auto destination=(phase==1?well:spring);
        const auto travel=(phase==1||phase==2||phase==4)?(progress<.5f?progress*2:(1-progress)*2):0;
        auto position=Ground(glm::mix(home,destination,travel));auto& transform=registry.Get<Transform>(creature);
        auto delta=position-transform.position;delta.y=0;
        if(glm::length(delta)>.01f)transform.rotation=glm::mat3(glm::eulerAngleY(std::atan2(delta.x,delta.z)));
        transform.position=position+glm::vec3(0,floorOffset+((phase==1||phase==2)?std::sin(progress*75)*.12f:0),0);
        auto& carried=registry.Get<Transform>(water);carried.position=position+glm::vec3(3,4,1);carried.scale=glm::vec3(phase==1&&progress>.45f&&progress<.98f?1.2f:0);
        if(deliveries>lastDelivery){
            const auto now=std::chrono::steady_clock::now();
            registry.Each<const Villager,const Transform>([&](entt::entity e,const Villager&,const Transform& t){if(glm::distance(t.position,home)<80)registry.AssignOrReplace<PlayerGiftReaction>(e,now,now+std::chrono::seconds(4));});lastDelivery=deliveries;
        }
        if(focus){const auto target=focus==2?position:home;Locator::camera::value().SetFocus(target+glm::vec3(0,4,0)).SetOrigin(target+(focus==2?glm::vec3(18,20,-28):glm::vec3(32,48,-65)));}
        registry.SetDirty();return 1;
    }
};
}

namespace openblack::browser {
static DroughtScene& Scene(){static DroughtScene scene;return scene;}
int RunDrought(int phase,float progress,int deliveries,int focus,int reset){return Scene().Run(phase,progress,deliveries,focus,reset);}
void RunDroughtWorker(int index,int job,float progress){Scene().Workers(index,job,progress);}
}
#endif
