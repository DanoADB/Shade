// Local L3D preview exporter; uses openblack's GPL-3.0 L3D reader.
#include "L3DFile.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <array>
using namespace openblack::l3d;
using nlohmann::json;
int main(int argc,char** argv){
 if(argc!=3){std::cerr<<"Usage: exporter input.l3d output.json\n";return 1;}
 L3DFile f; auto result=f.Open(argv[1]);if(result!=L3DResult::Success){std::cerr<<ResultToStr(result);return 2;}
 json out;out["textures"]=json::array();out["primitives"]=json::array();
 for(auto& skin:f.GetSkins()) {json tex;tex["id"]=skin.id;tex["rgba"]=json::array();for(auto p:skin.texels){tex["rgba"].push_back(p.r*17);tex["rgba"].push_back(p.g*17);tex["rgba"].push_back(p.b*17);tex["rgba"].push_back(p.a*17);}out["textures"].push_back(tex);}
 for(size_t sub=0;sub<f.GetSubmeshHeaders().size();sub++){
 if(f.GetSubmeshHeaders()[sub].flags.isPhysics || f.GetSubmeshHeaders()[sub].flags.status!=0 || (!f.GetSubmeshHeaders()[sub].flags.hasBones && f.GetSubmeshHeaders()[sub].flags.lodMask==0))continue;
 auto vertices=f.GetVertexSpan(sub);auto indices=f.GetIndexSpan(sub);auto groups=f.GetVertexGroupSpan(sub);auto bones=f.GetBoneSpan(sub);
 std::vector<L3DVertex> posed(vertices.begin(),vertices.end());size_t v=0;
 for(auto group:groups){for(unsigned j=0;j<group.vertexCount;j++,v++){
 if(v>=posed.size())return 3;auto& vertex=posed[v];unsigned bone=group.boneIndex;size_t steps=0;
 while(bone!=0xffffffffu){if(bone>=bones.size()||++steps>bones.size())return 4;auto& b=bones[bone];auto rotate=[&](L3DPoint p){auto& r=b.orientation;return L3DPoint{r[0]*p.x+r[3]*p.y+r[6]*p.z,r[1]*p.x+r[4]*p.y+r[7]*p.z,r[2]*p.x+r[5]*p.y+r[8]*p.z};};
 vertex.position=rotate(vertex.position);vertex.position.x+=b.position.x;vertex.position.y+=b.position.y;vertex.position.z+=b.position.z;vertex.normal=rotate(vertex.normal);bone=b.parent;
 }}}
 size_t vo=0,io=0;
 for(auto& p:f.GetPrimitiveSpan(sub)){json prim;prim["skin"]=p.material.skinID;prim["color"]={p.material.color.bgra.r,p.material.color.bgra.g,p.material.color.bgra.b,p.material.color.bgra.a};prim["vertices"]=json::array();prim["indices"]=json::array();
 for(size_t j=0;j<p.numVertices;j++){auto& v=posed.at(vo+j);for(float x:{v.position.x,v.position.y,v.position.z,v.texCoord.x,v.texCoord.y,v.normal.x,v.normal.y,v.normal.z})prim["vertices"].push_back(x);}
 for(size_t j=0;j<p.numTriangles*3;j++){auto ix=indices[io+j];if(ix>=p.numVertices)return 5;prim["indices"].push_back(ix);}
 out["primitives"].push_back(prim);vo+=p.numVertices;io+=p.numTriangles*3;
 }}
 std::ofstream stream(argv[2]);stream<<out;std::cout<<out["primitives"].size()<<" primitives, "<<out["textures"].size()<<" textures\n";return stream?0:6;
}

