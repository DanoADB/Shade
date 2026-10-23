#include "3D/CreatureAnimationBank.h"
#include "PackFile.h"
#include "L3DFile.h"
#include <cassert>
#include <iostream>
int main(){using namespace openblack;
for(const auto& choice:std::vector<std::pair<std::string,std::string>>{{"Bcow.CBN","C_Cow_Boned.l3d"},{"brhino.CBN","C_rhino_base.l3d"},{"bsheep.CBN","C_Sheep_Base.l3d"}}){
std::cout<<choice.first<<"\n";pack::PackFile pack;assert(pack.Open(std::filesystem::path(".local/native-game-data/Data/CTR")/choice.first)==pack::PackResult::Success);l3d::L3DFile mesh;assert(mesh.Open(std::filesystem::path(".local/native-game-data/Data/CreatureMesh")/choice.second)==l3d::L3DResult::Success);
 std::vector<glm::mat4> bind;std::vector<uint32_t> parents;for(const auto& b:mesh.GetBones()){glm::mat4 m(b.orientation[0],b.orientation[1],b.orientation[2],0,b.orientation[3],b.orientation[4],b.orientation[5],0,b.orientation[6],b.orientation[7],b.orientation[8],0,b.position.x,b.position.y,b.position.z,1);parents.push_back(b.parent);if(b.parent!=UINT32_MAX)m=bind[b.parent]*m;bind.push_back(m);}
 animation::CreatureAnimationBank bank;assert(bank.Load(pack.GetBlock("Creature"),".local/native-game-data/Data",bind,parents));assert(bank.Count()>100);assert(bank.TravelDistance("walk")>0);
 for(const char* name:{"stand","walk","start_sleep","sleep","end_sleep","drink","scatter","start_cast","cast","end_cast","happy"}){assert(bank.Contains(name));auto a=bank.Sample(name,0),b=bank.Sample(name,bank.Duration(name)*.37f);assert(a.size()==bind.size()&&b.size()==bind.size());float delta=0;for(size_t i=0;i<bind.size();++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k){assert(std::isfinite(b[i][j][k]));delta+=std::abs(a[i][j][k]-b[i][j][k]);}assert(std::string(name)=="stand"||delta>.01f);std::cout<<name<<" "<<bank.Duration(name)<<"ms, delta "<<delta<<"\n";}
 auto stand=bank.Sample("stand",0);
glm::vec3 lo(1e10f),hi(-1e10f);
 for(size_t sub=0;sub<mesh.GetSubmeshHeaders().size();++sub){auto vertices=mesh.GetVertexSpan(sub);size_t v=0;for(const auto& group:mesh.GetVertexGroupSpan(sub))for(unsigned j=0;j<group.vertexCount&&v<vertices.size();++j,++v){auto& p=vertices[v].position;auto q=stand[group.boneIndex]*glm::vec4(p.x,p.y,p.z,1);lo=glm::min(lo,glm::vec3(q));hi=glm::max(hi,glm::vec3(q));}}
 if(choice.first=="Bcow.CBN"){assert(hi.x-lo.x<30);assert(hi.y-lo.y>40&&hi.y-lo.y<55);}
 assert(hi.y>lo.y&&hi.x>lo.x); // Reject transposed/duplicated bind axes that fold or splay the rig.
 std::cout<<"Original creature motion bank "<<bank.Count()<<" names, all requested clips sampled on "<<bind.size()<<" bones\n";
}
}



