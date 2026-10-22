// Small deterministic fixtures for the shared DDS block decoder.
#include "Graphics/BlockTextureDecode.h"
#include <cassert>
#include <iostream>
using namespace openblack::graphics;
int main() {
    const std::array<uint8_t,8> red={0x00,0xf8,0x00,0x00,0,0,0,0};
    auto pixels=detail::DecodeBlockCompressed(TextureFormat::BlockCompression1,red,4,4,1);
    assert(pixels.size()==64);
    for(size_t i=0;i<16;++i)assert(pixels[i*4]==255 && pixels[i*4+1]==0 && pixels[i*4+2]==0 && pixels[i*4+3]==255);
    const std::array<uint8_t,8> transparent={0,0,0xff,0xff,0xff,0xff,0xff,0xff};
    pixels=detail::DecodeBlockCompressed(TextureFormat::BlockCompression1,transparent,1,1,1);
    assert(pixels.size()==4 && pixels[3]==0);
    std::array<uint8_t,16> bc2={};
    std::copy(red.begin(),red.end(),bc2.begin()+8);
    bc2[0]=0x8f;
    pixels=detail::DecodeBlockCompressed(TextureFormat::BlockCompression2,bc2,4,4,1);
    assert(pixels[0]==255 && pixels[3]==255 && pixels[7]==136);
    std::array<uint8_t,16> bc3={};
    std::copy(red.begin(),red.end(),bc3.begin()+8);bc3[0]=200;bc3[1]=100;
    pixels=detail::DecodeBlockCompressed(TextureFormat::BlockCompression3,bc3,4,4,1);
    assert(pixels[0]==255 && pixels[3]==200);
    std::cout<<"BC1 opaque/transparent, cropped edge, BC2 alpha, BC3 alpha passed\n";
}
