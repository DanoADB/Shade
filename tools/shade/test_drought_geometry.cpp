#include "3D/DroughtGeometry.h"
#include <cassert>
#include <iostream>
using namespace openblack::browser;
int main(){
 std::vector<glm::vec3> path={{0,0,0},{10,0,0},{20,0,5}};
 auto height=[](float x,float z){return x*.1f+z*.2f;};
 auto hidden=BuildWaterworks(path,{30,0,0},false,false,0,0,height);
 auto found=BuildWaterworks(path,{30,0,0},true,false,0,0,height);
 auto partial=BuildWaterworks(path,{30,0,0},true,false,.4f,0,height);
 auto done=BuildWaterworks(path,{30,0,0},true,true,0,0,height);
 assert(hidden.size()==144);assert(found.size()==288);assert(partial.size()>found.size());assert(done.size()>partial.size());
 assert(BuildWaterworks(path,{30,0,0},true,false,-1,0,height).size()==found.size());
 assert(BuildWaterworks(path,{30,0,0},true,false,2,0,height).size()>partial.size());
 for(auto v:done){assert(std::isfinite(v.position.y));assert(v.position.y>height(v.position.x,v.position.z));assert(v.color.a>0&&v.color.a<=1);}
 std::vector<glm::vec3> duplicate={{0,0,0},{0,0,0}};
 assert(BuildWaterworks(duplicate,{0,0,0},true,true,1,0,height).size()==found.size());
 std::cout<<"Waterworks discovery, construction progress, completion, terrain fit and degenerate paths passed\n";
}
