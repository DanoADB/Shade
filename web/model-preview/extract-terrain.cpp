#include "LNDFile.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){using namespace openblack::lnd;if(argc!=3)return 1;LNDFile f;auto result=f.Open(argv[1]);if(result!=LNDResult::Success){std::cerr<<ResultToStr(result);return 2;}std::filesystem::create_directories(argv[2]);int i=0;for(auto& m:f.GetMaterials()){std::ofstream out(std::string(argv[2])+"/"+std::to_string(i++)+".ppm",std::ios::binary);out<<"P6\n256 256\n255\n";for(auto p:m.texels){unsigned char rgb[3]={(unsigned char)(p.r*255/31),(unsigned char)(p.g*255/31),(unsigned char)(p.b*255/31)};out.write((char*)rgb,3);}std::cout<<i-1<<" type="<<m.type<<"\n";}}
