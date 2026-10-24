#ifdef __EMSCRIPTEN__
#include <algorithm>
#include <cmath>
#include <chrono>
#include <vector>
#include <memory>
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
#include "3D/CreatureAnimationBank.h"
#include "3D/TerrainRoutes.h"
#include "ECS/Components/Tree.h"
#include "PackFile.h"
#include "L3DFile.h"
#include "LNDFile.h"
#include "FileSystem/FileSystemInterface.h"
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
    std::array<std::vector<glm::vec3>,2> godRoutes;
    std::vector<std::array<std::vector<glm::vec3>,3>> workerRoutes;
    int routeMask=0;
    animation::CreatureAnimationBank motion;
    std::vector<std::pair<uint32_t,glm::vec3>> contacts;float standingLowest=0;
    std::vector<glm::mat4> pose,transitionPose;std::string motionName;
    float motionTime=0,motionSpeed=1,scenarioTime=0,blendStart=0,happyUntil=0,wakeUntil=0,castUntil=0;
    float walkedDistance=0,stepDistance=0,previousAnimateTime=0;
    int previousPhase=0,animationDeliveries=0;bool motionPaused=false,motionLoop=true;
    std::chrono::steady_clock::time_point motionClock;
    glm::vec3 Ground(glm::vec3 p)const {
        auto& terrain=Locator::terrainSystem::value();const float cell=LandIslandInterface::k_CellSize;
        const int x=int(std::floor(p.x/cell)),z=int(std::floor(p.z/cell));
        if(x<0||z<0||x>=511||z>=511){p.y=0;return p;}
        const auto& a=terrain.GetCell(glm::u16vec2(x,z));
        p.y=navigation::TriangleHeight(a.altitude,terrain.GetCell(glm::u16vec2(x+1,z)).altitude,terrain.GetCell(glm::u16vec2(x,z+1)).altitude,terrain.GetCell(glm::u16vec2(x+1,z+1)).altitude,a.properties.split,p.x/cell-x,p.z/cell-z)*LandIslandInterface::k_HeightUnit;
        return p;
    }
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
    bool BuildRoutes(){
        using namespace ecs::components;auto& registry=Locator::entitiesRegistry::value();
        std::vector<navigation::Obstacle> obstacles;
        registry.Each<const Fixed>([&](entt::entity e,const Fixed& fixed){
            if(glm::distance(fixed.boundingCenter,glm::vec2(home.x,home.z))>210+fixed.boundingRadius)return;
            // Tree Fixed bounds include the canopy. Navigation protects the scaled trunk.
            const float radius=registry.TryGet<Tree>(e)?std::max(.5f,fixed.boundingRadius*.22f):fixed.boundingRadius;
            obstacles.push_back({fixed.boundingCenter,radius});
        });
        auto height=[&](float x,float z){return Ground({x,0,z}).y;};
        auto origin=glm::vec2(home.x,home.z)-glm::vec2(160);
        navigation::TerrainRoutes large(origin,161,2,{3.2f,.6f},height,obstacles);
        glm::vec2 safeHome;if(!large.Nearest({home.x,home.z},safeHome,28))return false;
        home=Ground({safeHome.x,0,safeHome.y});
        emscripten_log(EM_LOG_CONSOLE,"Navigation sites: home %.1f/%.1f, well %.1f/%.1f, spring %.1f/%.1f",home.x,home.z,well.x,well.z,spring.x,spring.z);
        godRoutes[0]=large.Find(safeHome,{well.x,well.z},16);
        godRoutes[1]=large.Find(safeHome,{spring.x,spring.z},24);
        if(!godRoutes[1].empty())spring=godRoutes[1].back();
        channelPath=godRoutes[1];std::reverse(channelPath.begin(),channelPath.end());
        villagerNavigation=std::make_unique<navigation::TerrainRoutes>(origin,161,2,navigation::Profile{.65f,.65f},height,obstacles);
        auto& small=*villagerNavigation;
        workerRoutes.resize(workers.size());routeMask=0;
        if(!godRoutes[0].empty())routeMask|=1;
        if(!godRoutes[1].empty())routeMask|=2|4;
        for(size_t i=0;i<workers.size();++i){
            glm::vec2 start;if(!small.Nearest({workerHomes[i].x,workerHomes[i].z},start,16))continue;
            workerHomes[i]=Ground({start.x,0,start.y});registry.Get<Transform>(workers[i]).position=workerHomes[i];
            const std::array<glm::vec3,3> goals{well,home+glm::vec3(12,0,8),spring};
            for(int job=0;job<3;++job){workerRoutes[i][job]=small.Find(start,{goals[job].x,goals[job].z});if(!workerRoutes[i][job].empty())routeMask|=1<<(3+i*3+job);}
        }
        emscripten_log(EM_LOG_CONSOLE,"Drought navigation: %zu obstacles; route mask %d, creature waypoints %zu/%zu",obstacles.size(),routeMask,godRoutes[0].size(),godRoutes[1].size());
        return true;
    }
    struct GodChoice {CreatureType type;const char* bank;const char* mesh;};
    static constexpr std::array<GodChoice,3> gods{{
        {CreatureType::Cow,"Data/CTR/Bcow.CBN","Data/CreatureMesh/C_Cow_Boned.l3d"},
        {CreatureType::Rhino,"Data/CTR/brhino.CBN","Data/CreatureMesh/C_rhino_base.l3d"},
        {CreatureType::Sheep,"Data/CTR/bsheep.CBN","Data/CreatureMesh/C_Sheep_Base.l3d"}}};
    int selectedGod=0;
    int LoadGod(int selection){
        if(selection<0||size_t(selection)>=gods.size())return 0;
        const auto& choice=gods[selection];auto& meshes=Locator::resources::value().GetMeshes();
            const auto id=openblack::creature::GetIdFromType(choice.type,openblack::creature::CreatureBody::Appearance::Base);
            if(!meshes.Contains(id))return 0;
            auto mesh=meshes.Handle(id);
            animation::CreatureAnimationBank candidate;pack::PackFile pack;auto& fs=Locator::filesystem::value();
            const bool loaded=pack.Open(fs.FindPath(choice.bank))==pack::PackResult::Success&&pack.HasBlock("Creature")&&candidate.Load(pack.GetBlock("Creature"),fs.FindPath("Data/ctrspec27.txt").parent_path(),mesh->GetBoneMatrices(),mesh->GetBoneParents());
            if(!loaded){emscripten_log(EM_LOG_ERROR,"Selected creature animations could not load");return 0;}
            motion=std::move(candidate);standingLowest=0;droughtCreatureMesh=id;emscripten_log(EM_LOG_CONSOLE,"Original creature motion bank: %zu named clips, %zu joints",motion.Count(),mesh->GetBoneParents().size());
            auto box=mesh->GetBoundingBox();scale=11/std::max(.1f,box.Size().y);floorOffset=-box.minima.y*scale;
            contacts.clear();l3d::L3DFile originalMesh;
            if(originalMesh.Open(fs.FindPath(choice.mesh))==l3d::L3DResult::Success){
                const auto standing=motion.Sample("stand",0);float lowest=std::numeric_limits<float>::max(),highest=std::numeric_limits<float>::lowest();
                for(size_t sub=0;sub<originalMesh.GetSubmeshHeaders().size();++sub){auto vertices=originalMesh.GetVertexSpan(sub);size_t v=0;
                    for(const auto& group:originalMesh.GetVertexGroupSpan(sub))for(unsigned j=0;j<group.vertexCount&&v<vertices.size();++j,++v){if(group.boneIndex>=standing.size())continue;const auto& p=vertices[v].position;contacts.emplace_back(group.boneIndex,glm::vec3(p.x,p.y,p.z));
                        const float y=(standing[group.boneIndex]*glm::vec4(p.x,p.y,p.z,1)).y;lowest=std::min(lowest,y);highest=std::max(highest,y);}}
                if(std::isfinite(lowest)&&lowest<std::numeric_limits<float>::max()){standingLowest=lowest;scale=11/std::max(.1f,highest-lowest);floorOffset=-lowest*scale;}
            }
        selectedGod=selection;pose.clear();transitionPose.clear();motionName.clear();walkedDistance=stepDistance=0;
        happyUntil=wakeUntil=castUntil=0;previousPhase=0;
        return 1;
    }
public:
    std::unique_ptr<navigation::TerrainRoutes> villagerNavigation;
    int Routes()const{
        int mask=routeMask;auto& registry=Locator::entitiesRegistry::value();
        for(size_t i=0;i<workers.size();++i)if(registry.Valid(workers[i])&&registry.AllOf<ecs::components::HandCarried>(workers[i]))mask&=~(7<<(3+i*3));
        return mask;
    }
    bool PlaceVillager(entt::entity entity,glm::vec3& position){
        if(!villagerNavigation)return false;
        glm::vec2 safe;if(!villagerNavigation->Nearest({position.x,position.z},safe,8))return false;
        auto it=std::find(workers.begin(),workers.end(),entity);
        if(it!=workers.end()){
            const auto index=std::distance(workers.begin(),it);
            std::array<std::vector<glm::vec3>,3> paths;
            const std::array<glm::vec3,3> goals{well,home+glm::vec3(12,0,8),spring};int bits=0;
            for(int j=0;j<3;++j){paths[j]=villagerNavigation->Find(safe,{goals[j].x,goals[j].z});if(!paths[j].empty())bits|=1<<(3+index*3+j);}
            if(!bits)return false;
            workerRoutes[index]=std::move(paths);workerHomes[index]=Ground({safe.x,0,safe.y});
            routeMask=(routeMask&~(7<<(3+index*3)))|bits;
            EM_ASM({if(Module.shadeDroughtWorkerMoved)Module.shadeDroughtWorkerMoved($0);},index);
        }
        position=Ground({safe.x,0,safe.y});return true;
    }
    void RestoreCounters(int deliveries){lastDelivery=animationDeliveries=deliveries;happyUntil=wakeUntil=castUntil=0;}
    int SwapGod(int selection){
        auto& registry=Locator::entitiesRegistry::value();
        if(creature==entt::null||!registry.Valid(creature))return 0;
        if(selection==selectedGod)return 1;
        const float oldOffset=floorOffset;
        if(!LoadGod(selection))return 0;
        registry.Get<ecs::components::Mesh>(creature).id=droughtCreatureMesh;
        auto& transform=registry.Get<ecs::components::Transform>(creature);
        transform.position.y+=floorOffset-oldOffset;transform.scale=glm::vec3(scale);
        registry.SetDirty();return 1;
    }
    void Animate(int phase,float progress,float time,int paused,int deliveries){
        std::string next="stand";float tick=time*1000,speed=1;bool loop=true;
        if(deliveries>animationDeliveries){happyUntil=time+motion.Duration("happy")/1000.f;animationDeliveries=deliveries;}
        if(previousPhase==3&&phase!=3)wakeUntil=time+motion.Duration("end_sleep")/1000.f;
        if(previousPhase==5&&phase!=5)castUntil=time+motion.Duration("end_cast")/1000.f;
        if(phase==1||phase==2){next="walk";if(progress>=.4f&&progress<=.6f){next=phase==1?"drink":"stand";if(phase==1){tick=(progress-.4f)/.2f*motion.Duration("drink");speed=motion.Duration("drink")/3600.f;loop=false;}}}
        if(phase==3){tick=progress*12000;if(tick<motion.Duration("start_sleep")){next="start_sleep";loop=false;}else{next="sleep";tick-=motion.Duration("start_sleep");}}
        if(phase==4)next=progress<.2f?"walk":"scatter";
        if(phase==5){tick=progress*18000;if(tick<motion.Duration("start_cast")){next="start_cast";loop=false;}else{next="cast";tick-=motion.Duration("start_cast");}}
        if(phase==0&&time<happyUntil){next="happy";tick=(time-happyUntil)*1000+motion.Duration("happy");loop=false;}
        if(time<wakeUntil){next="end_sleep";tick=(time-wakeUntil)*1000+motion.Duration("end_sleep");loop=false;}
        if(time<castUntil){next="end_cast";tick=(time-castUntil)*1000+motion.Duration("end_cast");loop=false;}
        if(next=="walk"&&stepDistance<.001f)next="stand";
        if(next=="walk"){
            const float stride=std::max(.1f,motion.TravelDistance("walk")*scale),duration=float(motion.Duration("walk"));
            tick=walkedDistance/stride*duration;
            const float dt=time-previousAnimateTime;speed=dt>.0001f?std::clamp(stepDistance/dt/stride*duration/1000,0.f,4.f):0;
        }
        previousAnimateTime=time;
        if(next!=motionName){transitionPose=pose;blendStart=time;motionName=next;emscripten_log(EM_LOG_CONSOLE,"Creature animation: %s",motionName.c_str());}
        motionTime=tick;motionSpeed=speed;scenarioTime=time;motionPaused=paused!=0;motionLoop=loop;motionClock=std::chrono::steady_clock::now();previousPhase=phase;
    }
    const std::vector<glm::mat4>& Pose(){
        const float extra=motionPaused?0:std::clamp(std::chrono::duration<float>(std::chrono::steady_clock::now()-motionClock).count(),0.f,.2f);
        pose=motion.Sample(motionName,motionTime+extra*1000*motionSpeed,motionLoop);
        const float blend=std::clamp((scenarioTime+extra-blendStart)/.22f,0.f,1.f);
        if(blend<1&&transitionPose.size()==pose.size())for(size_t i=0;i<pose.size();++i){
            auto q=glm::slerp(glm::normalize(glm::quat_cast(glm::mat3(transitionPose[i]))),glm::normalize(glm::quat_cast(glm::mat3(pose[i]))),blend);
            auto p=glm::mix(glm::vec3(transitionPose[i][3]),glm::vec3(pose[i][3]),blend);pose[i]=glm::translate(glm::mat4(1),p)*glm::mat4_cast(q);
        }
        float lowest=std::numeric_limits<float>::max();
        for(const auto& [joint,point]:contacts)if(joint<pose.size())lowest=std::min(lowest,(pose[joint]*glm::vec4(point,1)).y);
        if(lowest<std::numeric_limits<float>::max()&&std::isfinite(lowest))for(auto& bone:pose)bone[3].y+=standingLowest-lowest;
        return pose;
    }
    void World(int discovered,int channel,float progress,float time){
        if(creature==entt::null||!Locator::entitiesRegistry::value().Valid(creature))return;
        droughtGround=BuildWaterworks(channelPath,well,discovered!=0,channel!=0,progress,time,[&](float x,float z){return Ground({x,0,z}).y;});++droughtGroundRevision;
    }
    void Workers(int index,int job,float progress){
        using namespace ecs::components;
        if(index<0||size_t(index)>=workers.size())return;
        auto& registry=Locator::entitiesRegistry::value();auto e=workers[index];if(!registry.Valid(e))return;
        if(registry.AllOf<HandCarried>(e)){
            if(size_t(index)<workerProps.size()&&registry.Valid(workerProps[index]))registry.Get<Transform>(workerProps[index]).scale=glm::vec3(0);
            return;
        }
        auto destination=job==0?well:job==1?home+glm::vec3(12,0,8):spring;
        const auto t=std::clamp(progress,0.f,1.f);const auto travel=t<.4f?t/.4f:t>.6f?(1-t)/.4f:1;
        auto& transform=registry.Get<Transform>(e);auto next=Ground(navigation::Along(workerRoutes[index][std::clamp(job,0,2)],travel,workerHomes[index]));auto delta=next-transform.position;delta.y=0;
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
        if(reset){for(auto e:props)if(registry.Valid(e))registry.Destroy(e);props.clear();workerProps.clear();workerRoutes.clear();for(auto& route:godRoutes)route.clear();routeMask=0;channelPath.clear();droughtGround.clear();++droughtGroundRevision;
            if(registry.Valid(creature))registry.Destroy(creature);if(registry.Valid(water))registry.Destroy(water);for(size_t i=0;i<workers.size();++i)if(registry.Valid(workers[i])){registry.Get<Transform>(workers[i]).position=workerHomes[i];registry.Remove<ScenarioActivity>(workers[i]);}
            workers.clear();workerHomes.clear();creature=water=entt::null;lastDelivery=0;animationDeliveries=0;previousPhase=0;happyUntil=wakeUntil=castUntil=0;pose.clear();transitionPose.clear();motionName.clear();walkedDistance=stepDistance=previousAnimateTime=0;}
        if(creature==entt::null){
            if(!LoadGod(selectedGod))return -3;
            const auto id=droughtCreatureMesh;auto mesh=meshes.Handle(id);
            home=Clearing({1920,0,2700});well=Clearing(home+glm::vec3(-28,0,-22));spring=Clearing(home+glm::vec3(32,0,15));
            // Pick a higher source so the visible watercourse descends toward the village.
            float best=-1e10f;
            for(int x=24;x<=56;x+=8)for(int z=-24;z<=32;z+=8){auto p=Clearing(home+glm::vec3(x,0,z));float gain=p.y-home.y;float score=-std::abs(gain-8.f)-glm::distance(p,home)*.02f;if(score>best){best=score;spring=p;}}
            for(int i=0;i<=16;++i){float t=i/16.f;channelPath.push_back(Ground(glm::mix(spring,home+glm::vec3(-4,0,3),t)+glm::vec3(std::sin(t*3.14159265f)*4,0,0)));}
            auto wellProp=Prop(MeshId::BuildingEgyptianWell,well+glm::vec3(-4,0,0),4);
            if(wellProp!=entt::null)registry.Assign<Fixed>(wellProp,glm::vec2(well.x-4,well.z),2.f);
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
            if(!BuildRoutes()){emscripten_log(EM_LOG_ERROR,"No safe creature clearing found");return -4;}
            emscripten_log(EM_LOG_CONSOLE,"Drought workers: %zu original villagers with autonomous jobs",workers.size());
            Locator::config::value().shadeDroughtScene=true;Locator::config::value().hidePlayerHand=false;focus=1;
            emscripten_log(EM_LOG_CONSOLE,"Drought creature: original model %s, %zu bones, scale %.3f",mesh->GetDebugName().c_str(),mesh->GetBoneMatrices().size(),scale);
        }
        if(!registry.Valid(creature))return -2;
        progress=std::clamp(progress,0.f,1.f);auto destination=(phase==1?well:spring);
        const auto travel=(phase==1||phase==2||phase==4)?(progress<.4f?progress/.4f:progress>.6f?(1-progress)/.4f:1):0;
        auto position=home;
        if(phase==1||phase==2)position=Ground(navigation::Along(godRoutes[phase-1],travel,home));
        if(phase==4){
            if(progress<.2f)position=Ground(navigation::Along(godRoutes[1],progress/.2f,home));
            else position=Ground(navigation::Along(channelPath,(progress-.2f)/.8f,home));
        }
        auto& transform=registry.Get<Transform>(creature);
        auto delta=position-transform.position;delta.y=0;stepDistance=glm::length(delta);walkedDistance+=stepDistance;
        // Original creature walk clips travel along local -Z; face that axis into the route.
        if(glm::length(delta)>.01f)transform.rotation=glm::mat3(glm::eulerAngleY(std::atan2(-delta.x,-delta.z)));
        transform.position=position+glm::vec3(0,floorOffset,0);
        auto& carried=registry.Get<Transform>(water);carried.position=position+transform.rotation*glm::vec3(3,4,1);carried.scale=glm::vec3(phase==1&&progress>.6f&&progress<.98f?1.2f:0);
        if(deliveries>lastDelivery){
            const auto now=std::chrono::steady_clock::now();
            registry.Each<const Villager,const Transform>([&](entt::entity e,const Villager&,const Transform& t){if(glm::distance(t.position,home)<80)registry.AssignOrReplace<PlayerGiftReaction>(e,now,now+std::chrono::seconds(4));});lastDelivery=deliveries;
        }
        if(focus){auto target=focus==2?position:focus==3?well:focus==4?spring:home;
            if(focus>=6&&size_t(focus-6)<workers.size()&&registry.Valid(workers[focus-6]))target=registry.Get<Transform>(workers[focus-6]).position;auto look=target+glm::vec3(0,4,0);auto eye=target+(focus>=2?glm::vec3(18,20,-28):glm::vec3(32,48,-65));
            // Keep the follow camera and its sightline above nearby hills.
            for(int i=0;i<16;++i){float t=i/16.f;auto p=glm::mix(eye,look,t);eye.y=std::max(eye.y,(Ground(p).y+2.5f-t*look.y)/(1-t));}
            Locator::camera::value().SetFocus(look).SetOrigin(eye);}
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
int DroughtRoutes(){return Scene().Routes();}
bool PlaceDroughtVillager(entt::entity entity,glm::vec3& position){return Scene().PlaceVillager(entity,position);}
int RunDrought(int phase,float progress,int deliveries,int focus,int reset){return Scene().Run(phase,progress,deliveries,focus,reset);}
void RestoreDroughtCounters(int deliveries){Scene().RestoreCounters(deliveries);}
int SwapDroughtGod(int selection){return Scene().SwapGod(selection);}
void RunDroughtWorker(int index,int job,float progress){Scene().Workers(index,job,progress);}
void AnimateDroughtCreature(int phase,float progress,float time,int paused,int deliveries){Scene().Animate(phase,progress,time,paused,deliveries);}
const std::vector<glm::mat4>& SampleDroughtCreature(){return Scene().Pose();}
void UpdateDroughtWorld(int discovered,int channel,float progress,float time){Scene().World(discovered,channel,progress,time);}
}
#endif
