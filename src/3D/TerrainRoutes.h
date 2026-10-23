#pragma once
#include <algorithm>
#include <array>
#include <glm/geometric.hpp>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
namespace openblack::navigation {
inline float TriangleHeight(float tl,float tr,float bl,float br,bool split,float x,float z){
    if(!split)return x>=z?tl+(tr-tl)*x+(br-tr)*z:tl+(br-bl)*x+(bl-tl)*z;
    return x+z<=1?tl+(tr-tl)*x+(bl-tl)*z:br+(bl-br)*(1-x)+(tr-br)*(1-z);
}
struct Obstacle {glm::vec2 center;float radius;};
struct Profile {float radius=.65f,maxSlope=.65f;};
// A bounded, deterministic terrain grid. No fallback ever crosses a rejected edge.
class TerrainRoutes {
    glm::vec2 origin;int side;float cell;Profile profile;
    std::function<float(float,float)> height;
    std::vector<Obstacle> obstacles;
    std::vector<uint8_t> passable;
    int Index(int x,int z)const{return z*side+x;}
    glm::vec2 Point(int i)const{return origin+glm::vec2(i%side,i/side)*cell;}
    bool Inside(glm::vec2 p)const{return p.x>=origin.x&&p.y>=origin.y&&p.x<=origin.x+(side-1)*cell&&p.y<=origin.y+(side-1)*cell;}
public:
    TerrainRoutes(glm::vec2 start,int count,float spacing,Profile actor,std::function<float(float,float)> terrain,std::vector<Obstacle> fixed)
        :origin(start),side(count),cell(spacing),profile(actor),height(std::move(terrain)),obstacles(std::move(fixed)){
        passable.resize(side*side);for(int i=0;i<side*side;++i)passable[i]=Clear(Point(i));
    }
    bool Clear(glm::vec2 p)const{
        if(!Inside(p))return false;
        for(const auto& o:obstacles)if(glm::length(p-o.center)<o.radius+profile.radius+.15f)return false;
        const float y=height(p.x,p.y);if(!std::isfinite(y)||y<.25f)return false;
        const float r=std::max(.5f,profile.radius);
        for(const auto d:std::array<glm::vec2,8>{{{r,0},{-r,0},{0,r},{0,-r},{r*.7071f,r*.7071f},{-r*.7071f,r*.7071f},{r*.7071f,-r*.7071f},{-r*.7071f,-r*.7071f}}}){
            float other=height(p.x+d.x,p.y+d.y);if(!std::isfinite(other)||other<.25f||std::abs(other-y)>r*profile.maxSlope)return false;
        }
        return true;
    }
    bool Segment(glm::vec2 a,glm::vec2 b)const{
        const auto delta=b-a;const float squared=glm::dot(delta,delta);
        for(const auto& o:obstacles){const float t=squared>0?std::clamp(glm::dot(o.center-a,delta)/squared,0.f,1.f):0;
            if(glm::length(a+delta*t-o.center)<o.radius+profile.radius+.15f)return false;}
        const float length=glm::length(delta);const int steps=std::max(1,int(std::ceil(length/.5f)));
        float previous=height(a.x,a.y);
        for(int i=0;i<=steps;++i){auto p=a+(b-a)*(float(i)/steps);if(!Clear(p))return false;float y=height(p.x,p.y);
            if(i&&std::abs(y-previous)>length/steps*profile.maxSlope+.001f)return false;previous=y;
        }
        return true;
    }
    bool Nearest(glm::vec2 desired,glm::vec2& result,float reach=16)const{
        float best=reach*reach;bool found=false;
        for(int i=0;i<side*side;++i)if(passable[i]){const auto p=Point(i);auto delta=p-desired;float d=glm::dot(delta,delta);if(d<=best){best=d;result=p;found=true;}}
        return found;
    }
    std::vector<glm::vec3> Find(glm::vec2 from,glm::vec2 goal,float approach=12)const{
        if(!Clear(from))return {};
        // Choose a safe approach point, never the interior of an obstacle.
        glm::vec2 target;if(!Nearest(goal,target,approach))return {};
        glm::vec2 start;if(!Nearest(from,start,cell*1.5f)||!Segment(from,start))return {};
        auto node=[&](glm::vec2 p){return Index(int(std::round((p.x-origin.x)/cell)),int(std::round((p.y-origin.y)/cell)));};
        const int first=node(start),count=side*side;int last=node(target);
        struct Open {float score;int node;bool operator<(const Open& b)const{return score==b.score?node>b.node:score>b.score;}};
        std::priority_queue<Open> open;std::vector<float> cost(count,std::numeric_limits<float>::infinity());std::vector<int> parent(count,-1);std::vector<uint8_t> closed(count);
        cost[first]=0;open.push({glm::length(start-target),first});
        while(!open.empty()){
            int current=open.top().node;open.pop();if(closed[current])continue;closed[current]=1;if(current==last)break;
            int x=current%side,z=current/side;
            for(int dz=-1;dz<=1;++dz)for(int dx=-1;dx<=1;++dx){if((!dx&&!dz)||x+dx<0||z+dz<0||x+dx>=side||z+dz>=side)continue;
                int next=Index(x+dx,z+dz);if(closed[next]||!passable[next])continue;
                if(dx&&dz&&(!passable[Index(x+dx,z)]||!passable[Index(x,z+dz)]))continue;
                auto a=Point(current),b=Point(next);if(!Segment(a,b))continue;
                float nextCost=cost[current]+glm::length(b-a)+std::abs(height(a.x,a.y)-height(b.x,b.y))*.5f;
                if(nextCost<cost[next]){cost[next]=nextCost;parent[next]=current;open.push({nextCost+glm::length(b-target),next});}
            }
        }
        if(!closed[last]){
            // The closest clear node can belong to another disconnected area.
            // Use the closest approach that was actually reached, or report blocked.
            float nearest=approach;last=-1;
            for(int i=0;i<count;++i)if(closed[i]){float distance=glm::length(Point(i)-goal);if(distance<=nearest){nearest=distance;last=i;}}
            if(last<0)return {};
        }
        std::vector<glm::vec2> raw;for(int i=last;i!=-1;i=parent[i])raw.push_back(Point(i));std::reverse(raw.begin(),raw.end());raw.insert(raw.begin(),from);
        std::vector<glm::vec3> path;size_t at=0;
        while(at<raw.size()){auto p=raw[at];path.emplace_back(p.x,height(p.x,p.y),p.y);if(at==raw.size()-1)break;size_t next=at+1;while(next+1<raw.size()&&Segment(raw[at],raw[next+1]))++next;at=next;}
        return path;
    }
};
inline glm::vec3 Along(const std::vector<glm::vec3>& path,float progress,glm::vec3 fallback){
    if(path.empty())return fallback;if(path.size()==1)return path.front();
    float total=0;for(size_t i=1;i<path.size();++i)total+=glm::length(path[i]-path[i-1]);
    float distance=std::clamp(progress,0.f,1.f)*total;
    for(size_t i=1;i<path.size();++i){float length=glm::length(path[i]-path[i-1]);if(distance<=length&&length>0)return path[i-1]+(path[i]-path[i-1])*(distance/length);distance-=length;}
    return path.back();
}
}
