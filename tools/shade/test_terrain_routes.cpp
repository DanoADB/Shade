#include "3D/TerrainRoutes.h"
#include <cassert>
#include <iostream>
using namespace openblack::navigation;
int main(){
 for(bool split:{false,true}){assert(std::abs(TriangleHeight(10,20,30,40,split,.2f,.3f)-18)<.001f);assert(std::abs(TriangleHeight(10,20,30,40,split,.8f,.9f)-36)<.001f);}
 assert(TriangleHeight(0,0,0,10,false,.5f,.5f)==5);assert(TriangleHeight(0,0,0,10,true,.5f,.5f)==0);
 auto flat=[](float,float){return 10.f;};
 TerrainRoutes grid({-30,-30},61,1,{1,.6f},flat,{{{0,0},5},{{10,0},1}});
 auto route=grid.Find({-20,0},{20,0});assert(route.size()>2);
 for(size_t i=1;i<route.size();++i)assert(grid.Segment({route[i-1].x,route[i-1].z},{route[i].x,route[i].z}));
 for(int i=0;i<=1000;++i){auto p=Along(route,i/1000.f,{});assert(glm::length(glm::vec2(p.x,p.z))>=6.15f-.001f);}
 auto approach=grid.Find({-20,0},{0,0});assert(!approach.empty());assert(glm::length(glm::vec2(approach.back().x,approach.back().z))>=6.15f);
 TerrainRoutes wall({-20,-20},41,1,{.6f,.6f},[](float x,float){return x>0?30.f:10.f;},{});assert(wall.Find({-10,0},{10,0},1).empty());
 auto reachableApproach=wall.Find({-10,0},{5,0},8);assert(!reachableApproach.empty());assert(reachableApproach.back().x<0);
 TerrainRoutes hill({-20,-20},41,1,{.6f,.6f},[](float x,float z){return 10.f+std::max(0.f,5.f-std::abs(x))*std::max(0.f,5.f-std::abs(z));},{});
 auto around=hill.Find({-15,0},{15,0});assert(!around.empty());for(size_t i=1;i<around.size();++i)assert(hill.Segment({around[i-1].x,around[i-1].z},{around[i].x,around[i].z}));
 TerrainRoutes water({-10,-10},21,1,{.6f,.6f},[](float x,float){return std::abs(x)<2?0.f:10.f;},{});assert(water.Find({-8,0},{8,0},1).empty());
 TerrainRoutes narrowSmall({-10,-10},21,1,{.5f,.6f},flat,{{{0,-5},3},{{0,5},3}}),narrowLarge({-10,-10},21,1,{2.5f,.6f},flat,{{{0,-5},3},{{0,5},3}});
 assert(narrowSmall.Segment({-8,0},{8,0}));assert(!narrowLarge.Segment({-8,0},{8,0}));
 assert(grid.Find({0,0},{20,0}).empty());assert(Along({},.5f,{1,2,3})==glm::vec3(1,2,3));
 std::cout<<"Terrain routes avoid obstacles, corners, cliffs and water; clearance, safe approaches, unreachable stops and arc-length sampling passed\n";
}
