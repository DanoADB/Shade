#ifdef __EMSCRIPTEN__
#include <algorithm>
#include <cmath>
#include <chrono>
#include <vector>
#include <array>
#include "Windowing/WindowingInterface.h"
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
    std::vector<entt::entity> props,workerProps;std::vector<glm::vec3> channelPath;
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
    entt::entity Prop(MeshId id,glm::vec3 position,float height){
        auto& registry=Locator::entitiesRegistry::value();auto& meshes=Locator::resources::value().GetMeshes();auto key=resources::HashIdentifier(id);
        if(!meshes.Contains(key))return entt::null;
        auto box=meshes.Handle(key)->GetBoundingBox();float s=height/std::max(.01f,box.Size().y);
        auto e=registry.Create();registry.Assign<ecs::components::Mesh>(e,key,int8_t(-1),int8_t(-1));
        registry.Assign<ecs::components::Transform>(e,position+glm::vec3(0,-box.minima.y*s,0),glm::mat3(1),glm::vec3(s));props.push_back(e);return e;
    }
public:
    void World(int discovered,int channel,float progress,float time){
        if(creature==entt::null||!Locator::entitiesRegistry::value().Valid(creature))return;
        droughtGround=BuildWaterworks(channelPath,well,discovered!=0,channel!=0,progress,time,[&](float x,float z){return Ground({x,0,z}).y;});++droughtGroundRevision;
    }
    void Workers(int index,int job,float progress){
        using namespace ecs::components;
        if(index<0||size_t(index)>=workers.size())return;
        auto& registry=Locator::entitiesRegistry::value();auto e=workers[index];if(!registry.Valid(e))return;
        auto destination=job==0?well:job==1?home+glm::vec3(12,0,8):spring;
        const auto t=std::clamp(progress,0.f,1.f);const auto travel=t<.4f?t/.4f:t>.6f?(1-t)/.4f:1;
        auto& transform=registry.Get<Transform>(e);auto next=Ground(glm::mix(workerHomes[index],destination,travel));auto delta=next-transform.position;delta.y=0;
        if(glm::length(delta)>.01f)transform.rotation=glm::mat3(glm::eulerAngleY(std::atan2(delta.x,delta.z)));
        transform.position=next;
        if(size_t(index)<workerProps.size()&&registry.Valid(workerProps[index])){
            const auto id=job==0?MeshId::U_Bucket:job==1?MeshId::I_OfferingFood:MeshId::I_OfferingWood;
            auto& meshes=Locator::resources::value().GetMeshes();const auto key=resources::HashIdentifier(id);
            if(meshes.Contains(key)){
                auto& m=registry.Get<Mesh>(workerProps[index]);m.id=key;
                auto& prop=registry.Get<Transform>(workerProps[index]);auto box=meshes.Handle(key)->GetBoundingBox();
                const float s=.8f/std::max(.01f,box.Size().y);prop.position=next+glm::vec3(.5f,1.1f,0);prop.rotation=transform.rotation;prop.scale=glm::vec3(t>.56f&&t<.99f?s:0);
            }
        }
        registry.AssignOrReplace<ScenarioActivity>(e,glm::length(delta)>.01f);registry.SetDirty();
    }
    int Run(int phase,float progress,int deliveries,int focus,int reset) {
        using namespace ecs::components;
        if(!Locator::entitiesRegistry::has_value()||!Locator::terrainSystem::has_value())return 0;
        auto& registry=Locator::entitiesRegistry::value();auto& meshes=Locator::resources::value().GetMeshes();
        if(reset){for(auto e:props)if(registry.Valid(e))registry.Destroy(e);props.clear();workerProps.clear();channelPath.clear();droughtGround.clear();++droughtGroundRevision;
            if(registry.Valid(creature))registry.Destroy(creature);if(registry.Valid(water))registry.Destroy(water);for(size_t i=0;i<workers.size();++i)if(registry.Valid(workers[i])){registry.Get<Transform>(workers[i]).position=workerHomes[i];registry.Remove<ScenarioActivity>(workers[i]);}
            workers.clear();workerHomes.clear();creature=water=entt::null;lastDelivery=0;}
        if(creature==entt::null){
            const auto id=openblack::creature::GetIdFromType(CreatureType::Cow,openblack::creature::CreatureBody::Appearance::Base);
            if(!meshes.Contains(id))return -1;
            auto mesh=meshes.Handle(id);auto box=mesh->GetBoundingBox();scale=11/std::max(.1f,box.Size().y);floorOffset=-box.minima.y*scale;
            home=Clearing({1920,0,2700});well=Clearing(home+glm::vec3(-28,0,-22));spring=Clearing(home+glm::vec3(32,0,15));
            // Pick a higher source so the visible watercourse descends toward the village.
            float best=-1e10f;
            for(int x=24;x<=56;x+=8)for(int z=-24;z<=32;z+=8){auto p=Clearing(home+glm::vec3(x,0,z));float gain=p.y-home.y;float score=-std::abs(gain-8.f)-glm::distance(p,home)*.02f;if(score>best){best=score;spring=p;}}
            for(int i=0;i<=16;++i){float t=i/16.f;channelPath.push_back(Ground(glm::mix(spring,home+glm::vec3(-4,0,3),t)+glm::vec3(std::sin(t*3.14159265f)*4,0,0)));}
            Prop(MeshId::BuildingEgyptianWell,well+glm::vec3(-4,0,0),4);
            for(int i=0;i<6;++i){float a=i*6.2831853f/6;Prop(MeshId::ObjectFlatRockLimeStone,Ground(well+glm::vec3(std::cos(a)*3.6f,0,std::sin(a)*3.6f)),.65f);}
            creature=registry.Create();registry.Assign<Mesh>(creature,id,int8_t(-1),int8_t(-1));registry.Assign<Transform>(creature,home+glm::vec3(0,floorOffset,0),glm::mat3(1),glm::vec3(scale));
            water=registry.Create();registry.Assign<Mesh>(water,resources::HashIdentifier(MeshId::U_Bucket),int8_t(0),int8_t(-1));registry.Assign<Transform>(water,home,glm::mat3(1),glm::vec3(0));
            std::vector<std::pair<float,entt::entity>> candidates;
            registry.Each<const Villager,const Transform>([&](entt::entity e,const Villager&,const Transform& t){
                bool outside=true;registry.Each<const Fixed>([&](const auto& f){if(glm::distance(glm::vec2(t.position.x,t.position.z),f.boundingCenter)<f.boundingRadius+1)outside=false;});
                if(outside)candidates.emplace_back(glm::distance(t.position,home),e);
            });
            std::sort(candidates.begin(),candidates.end());
            for(size_t i=0;i<std::min(size_t(3),candidates.size());++i){workers.push_back(candidates[i].second);workerHomes.push_back(registry.Get<Transform>(workers.back()).position);workerProps.push_back(Prop(MeshId::U_Bucket,workerHomes.back(),.8f));}
            emscripten_log(EM_LOG_CONSOLE,"Drought workers: %zu original villagers with autonomous jobs",workers.size());
            Locator::config::value().hidePlayerHand=true;focus=1;
            emscripten_log(EM_LOG_CONSOLE,"Drought creature: original cow %s, %zu bones, scale %.3f",mesh->GetDebugName().c_str(),mesh->GetBoneMatrices().size(),scale);
        }
        if(!registry.Valid(creature))return -2;
        progress=std::clamp(progress,0.f,1.f);auto destination=(phase==1?well:spring);
        const auto travel=(phase==1||phase==2||phase==4)?(progress<.5f?progress*2:(1-progress)*2):0;
        auto position=Ground(glm::mix(home,destination,travel));
        if(phase==4){
            if(progress<.2f)position=Ground(glm::mix(home,spring,progress/.2f));
            else{float t=std::clamp((progress-.2f)/.8f,0.f,1.f)*(channelPath.size()-1);size_t i=std::min(size_t(t),channelPath.size()-2);position=Ground(glm::mix(channelPath[i],channelPath[i+1],t-float(i)));}
        }
        auto& transform=registry.Get<Transform>(creature);
        auto delta=position-transform.position;delta.y=0;
        if(glm::length(delta)>.01f)transform.rotation=glm::mat3(glm::eulerAngleY(std::atan2(delta.x,delta.z)));
        transform.position=position+glm::vec3(0,floorOffset+((phase==1||phase==2)?std::sin(progress*75)*.12f:0),0);
        auto& carried=registry.Get<Transform>(water);carried.position=position+transform.rotation*glm::vec3(3,4,1);carried.scale=glm::vec3(phase==1&&progress>.45f&&progress<.98f?1.2f:0);
        if(deliveries>lastDelivery){
            const auto now=std::chrono::steady_clock::now();
            registry.Each<const Villager,const Transform>([&](entt::entity e,const Villager&,const Transform& t){if(glm::distance(t.position,home)<80)registry.AssignOrReplace<PlayerGiftReaction>(e,now,now+std::chrono::seconds(4));});lastDelivery=deliveries;
        }
        if(focus){auto target=focus==2?position:focus==3?well:focus==4?spring:home;
            if(focus>=6&&size_t(focus-6)<workers.size()&&registry.Valid(workers[focus-6]))target=registry.Get<Transform>(workers[focus-6]).position;Locator::camera::value().SetFocus(target+glm::vec3(0,4,0)).SetOrigin(target+(focus>=2?glm::vec3(18,20,-28):glm::vec3(32,48,-65)));}
        std::array<float,18> anchors{};const auto size=Locator::windowing::value().GetSize();
        for(int i=0;i<6;++i){auto p=i==0?home:i==1?well:spring;
            if(i>=3){if(size_t(i-3)>=workers.size()||!registry.Valid(workers[i-3]))continue;p=registry.Get<Transform>(workers[i-3]).position;}
            glm::vec3 screen(-1);const bool visible=Locator::camera::value().ProjectWorldToScreen(p+glm::vec3(0,2,0),{0,0,size.x,size.y},screen);
            anchors[i*3]=screen.x/size.x;anchors[i*3+1]=screen.y/size.y;anchors[i*3+2]=visible&&screen.x>=0&&screen.x<size.x&&screen.y>=0&&screen.y<size.y?1:0;
        }
        EM_ASM({if(Module.shadeDroughtAnchors){const a=[];for(let i=0;i<6;i++){const k=($0>>2)+i*3;a.push([HEAPF32[k],HEAPF32[k+1],HEAPF32[k+2]]);}Module.shadeDroughtAnchors(a);}},anchors.data());
        registry.SetDirty();return 1;
    }
};
}

namespace openblack::browser {
static DroughtScene& Scene(){static DroughtScene scene;return scene;}
int RunDrought(int phase,float progress,int deliveries,int focus,int reset){return Scene().Run(phase,progress,deliveries,focus,reset);}
void RunDroughtWorker(int index,int job,float progress){Scene().Workers(index,job,progress);}
void UpdateDroughtWorld(int discovered,int channel,float progress,float time){Scene().World(discovered,channel,progress,time);}
}
#endif
