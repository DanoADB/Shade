// Shade browser gameplay: uses the loaded engine registry, camera and original art.
#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <map>
#include <string>
#include <vector>
#include <emscripten/emscripten.h>
#include "Camera/Camera.h"
#include "ECS/Registry.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/PlayerGiftReaction.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "Resources/ResourceManager.h"
#include "3D/AllMeshes.h"
#include "Locator.h"

namespace openblack::browser
{
using namespace ecs::components;
class Gameplay
{
    entt::entity selected{entt::null}, bundle{entt::null}, source{entt::null};
    std::unordered_set<uint32_t> gathered;
    std::map<uint32_t,uint32_t> savedStocks;
    entt::entity responder{entt::null};
    struct SavedObject {int kind;uint32_t id;glm::vec3 position;uint32_t amount;};
    std::vector<SavedObject> pending;
    bool restoreValid{true};
    unsigned delivered{0};
    const char* message{"Gather wood from three trees and deliver it to village buildings."};
    int Kind(entt::entity entity) const
    {
        const auto& registry=Locator::entitiesRegistry::value();
        if (!registry.Valid(entity) || !registry.AllOf<Transform>(entity)) return 0;
        if (registry.AllOf<Tree>(entity)) return 1;
        if (registry.AllOf<Abode>(entity)) return 2;
        if (registry.AllOf<Villager>(entity)) return 3;
        return 0;
    }
    glm::vec3 Anchor(entt::entity entity) const
    {
        return Locator::entitiesRegistry::value().Get<Transform>(entity).position+
               glm::vec3(0,Kind(entity)==3 ? 1.0f : 3.0f,0);
    }
    bool Outdoors(const glm::vec3& position) const
    {
        bool outside=true;
        Locator::entitiesRegistry::value().Each<const Abode,const Fixed>([&](const auto&,const Fixed& fixed){
            if(glm::distance(glm::vec2(position.x,position.z),fixed.boundingCenter)<fixed.boundingRadius+2.0f)outside=false;
        });
        return outside;
    }
    void React(entt::entity entity)
    {
        auto& registry=Locator::entitiesRegistry::value();
        const auto now=std::chrono::steady_clock::now();
        const auto duration=registry.Get<Villager>(entity).sex==Villager::Sex::FEMALE ? 7333 : 4266;
        registry.AssignOrReplace<PlayerGiftReaction>(entity,now,now+std::chrono::milliseconds(duration));
    }
    void Focus(int kind)
    {
        auto& registry=Locator::entitiesRegistry::value();
        const auto center=Locator::camera::value().GetFocus();
        float nearest=std::numeric_limits<float>::max();
        entt::entity candidate=entt::null;
        registry.Each<const Transform>([&](entt::entity entity,const Transform& transform) {
            if(Kind(entity)!=kind || (kind==1 && gathered.contains(uint32_t(entity)))) return;
            if(kind==3 && !Outdoors(transform.position))return;
            const auto delta=transform.position-center;
            const float distance=glm::dot(delta,delta);
            if(distance<nearest){nearest=distance;candidate=entity;}
        });
        selected=candidate;
        if(!Kind(selected)){message="No matching object nearby.";return;}
        const auto anchor=Anchor(selected);
        Locator::camera::value().SetFocus(anchor).SetOrigin(anchor+glm::vec3(20,22,-30));
        message="Object selected. Choose an action below.";
    }
    void Select(float x,float y)
    {
        selected=entt::null;
        if(!std::isfinite(x)||!std::isfinite(y)||x<0||x>1||y<0||y>1)return;
        float best=44.0f*44.0f;
        auto& camera=Locator::camera::value();
        const auto size=Locator::windowing::value().GetSize();
        Locator::entitiesRegistry::value().Each<const Transform>([&](entt::entity entity,const Transform&) {
            if(!Kind(entity))return;
            glm::vec3 projected;
            if(!camera.ProjectWorldToScreen(Anchor(entity),{0,0,size.x,size.y},projected))return;
            const glm::vec2 delta=glm::vec2(projected)-glm::vec2(x*size.x,y*size.y);
            const auto distance=glm::dot(delta,delta);
            if(distance<best){best=distance;selected=entity;}
        });
        message=Kind(selected) ? "Object selected." : "No object here. Tap a tree, villager or building, or use Find.";
    }
public:
    void Save()
    {
        const auto& registry=Locator::entitiesRegistry::value();
        std::string json="{\"version\":1,\"objects\":[";
        bool first=true;
        auto add=[&](int kind,uint32_t id,uint32_t amount) {
            const auto entity=entt::entity(id);
            if(!registry.Valid(entity))return;
            const auto position=registry.Get<Transform>(entity).position;
            if(!first)json+=",";first=false;
            json+="["+std::to_string(kind)+","+std::to_string(id)+","+std::to_string(position.x)+","+
                std::to_string(position.y)+","+std::to_string(position.z)+","+std::to_string(amount)+"]";
        };
        for(const auto id:gathered)if(entt::entity(id)!=source)add(1,id,0);
        for(const auto& [id,amount]:savedStocks)add(2,id,amount);
        json+="]}";
        EM_ASM({if(Module.shadeGameplaySave)Module.shadeGameplaySave(UTF8ToString($0));},json.c_str());
    }
    int Restore(int kind,uint32_t id,float x,float y,float z,uint32_t amount)
    {
        if(!Locator::entitiesRegistry::has_value())return 0;
        auto& registry=Locator::entitiesRegistry::value();
        if(kind==0){pending.clear();restoreValid=delivered==0 && bundle==entt::null;return restoreValid;}
        if(kind==3)
        {
            if(!restoreValid)return 0;
            std::unordered_set<uint32_t> trees,buildings;
            for(const auto& entry:pending)
            {
                auto& ids=entry.kind==1?trees:buildings;
                if(!ids.insert(entry.id).second)return 0;
            }
            for(const auto& entry:pending)
                if(entry.kind==1)gathered.insert(entry.id);
                else {registry.Get<Abode>(entt::entity(entry.id)).woodAmount=entry.amount;savedStocks[entry.id]=entry.amount;}
            delivered=trees.size()*10;
            message="Saved village task restored. Any carried wood was returned to its tree.";
            pending.clear();return 1;
        }
        const auto entity=entt::entity(id);
        const glm::vec3 position(x,y,z);
        if(!restoreValid || (kind!=1 && kind!=2) || pending.size()>=10000 || Kind(entity)!=kind ||
           !std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||
           glm::distance(registry.Get<Transform>(entity).position,position)>0.01f)
        {restoreValid=false;return 0;}
        pending.push_back({kind,id,position,amount});return 1;
    }
    void Run(int action,float x,float y)
    {
        if(!Locator::entitiesRegistry::has_value()||!Locator::camera::has_value())return;
        auto& registry=Locator::entitiesRegistry::value();
        const auto now=std::chrono::steady_clock::now();
        std::vector<entt::entity> expired;
        registry.Each<const PlayerGiftReaction>([&](entt::entity entity,const auto& reaction){if(now>=reaction.until)expired.push_back(entity);});
        for(const auto entity:expired)registry.Remove<PlayerGiftReaction>(entity);
        if(bundle!=entt::null && !registry.Valid(bundle)){bundle=entt::null;source=entt::null;}
        if(action==1)Select(x,y);
        if(action>=4 && action<=6)Focus(action-3);
        if(action==8 && Kind(responder)==3)
        {
            selected=responder;
            const auto anchor=Anchor(selected);
            Locator::camera::value().SetFocus(anchor).SetOrigin(anchor+glm::vec3(3,3,-5));
            React(selected);
            message="A nearby villager thanks you for the delivery.";
        }
        if(action==2)
        {
            if(bundle!=entt::null)message="Deliver or return the wood in your hand first.";
            else if(Kind(selected)!=1)message="Select a tree first.";
            else if(gathered.contains(uint32_t(selected)))message="This tree has already supplied wood.";
            else
            {
                source=selected;
                bundle=registry.Create();
                registry.Assign<Transform>(bundle,Anchor(selected),glm::mat3(1),glm::vec3(1));
                registry.Assign<Mesh>(bundle,resources::HashIdentifier(MeshId::ObjectWoodInHand),int8_t(0),int8_t(-1));
                gathered.insert(uint32_t(source));
                message="Carrying 10 wood. Select a village building and deliver it.";
            }
        }
        if(action==3)
        {
            if(bundle==entt::null)message="Gather wood from a tree first.";
            else if(Kind(selected)!=2)message="Select a village building to receive the wood.";
            else
            {
                auto& stock=registry.Get<Abode>(selected).woodAmount;
                if(stock>std::numeric_limits<uint32_t>::max()-10)message="This building cannot store more wood.";
                else
                {
                    stock+=10;delivered+=10;
                    savedStocks[uint32_t(selected)]=stock;
                    registry.Destroy(bundle);bundle=entt::null;source=entt::null;
                    message=delivered>=30 ? "Village supplied! You delivered 30 wood. Keep helping or explore." : "Delivered 10 wood to this building.";
                    float nearest=120.0f*120.0f;responder=entt::null;
                    const auto target=registry.Get<Transform>(selected).position;
                    registry.Each<const Villager,const Transform>([&](entt::entity entity,const auto&,const auto& transform){
                        if(!Outdoors(transform.position))return;
                        const auto delta=target-transform.position;const auto distance=glm::dot(delta,delta);
                        if(distance<nearest){nearest=distance;responder=entity;}
                    });
                    if(Kind(responder)==3)React(responder);
                    Save();
                }
            }
        }
        if(action==7 && bundle!=entt::null)
        {
            gathered.erase(uint32_t(source));registry.Destroy(bundle);bundle=entt::null;source=entt::null;
            message="Wood returned to its tree. Nothing was delivered.";
        }
        if(bundle!=entt::null && Locator::handSystem::has_value())
        {
            const auto hands=Locator::handSystem::value().GetPlayerHandPositions();
            if(hands[0]){registry.Get<Transform>(bundle).position=*hands[0]+glm::vec3(0,0.5f,0);registry.SetDirty();}
        }
        const int kind=Kind(selected);
        unsigned wood=0,food=0,people=0,health=0,hunger=0;
        if(kind==2){const auto& abode=registry.Get<Abode>(selected);wood=abode.woodAmount;food=abode.foodAmount;people=abode.inhabitants.size();}
        if(kind==3){const auto& villager=registry.Get<Villager>(selected);health=villager.health;hunger=villager.hunger;}
        glm::vec3 marker(-1);
        const auto size=Locator::windowing::value().GetSize();
        if(kind && !Locator::camera::value().ProjectWorldToScreen(Anchor(selected),{0,0,size.x,size.y},marker))marker=glm::vec3(-1);
        EM_ASM({if(Module.shadeGameplayUpdate)Module.shadeGameplayUpdate({kind:$0,carrying:!!$1,delivered:$2,wood:$3,food:$4,people:$5,health:$6,hunger:$7,
            gathered:!!$8,message:UTF8ToString($9),x:$10,y:$11,response:!!$12});},kind,bundle!=entt::null,delivered,wood,food,people,health,hunger,
            kind==1 && gathered.contains(uint32_t(selected)),message,marker.x/size.x,marker.y/size.y,Kind(responder)==3);
    }
};
}
