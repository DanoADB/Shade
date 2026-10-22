// Exports openblack LND terrain cells as a simple XYZ RGB vertex stream.
// Does not redistribute game assets. Build using components/lnd CMake target.
#include <LNDFile.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>

using namespace openblack::lnd;
static void point(std::ostream& out, const LNDBlock& b, int x, int z) {
    const auto& cell = b.cells.at(z * 17 + x);
    // Height units / horizontal scale match the original data only approximately;
    // verify against LandBlock::BuildVertexList before production use.
    const float px = b.mapX + static_cast<float>(x);
    const float pz = b.mapZ + static_cast<float>(z);
    const float py = static_cast<float>(cell.altitude) * 0.1f;
    out << px << ' ' << py << ' ' << pz << ' '
        << cell.r/255.f << ' ' << cell.g/255.f << ' ' << cell.b/255.f << '\n';
}
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: shade-export-lnd INPUT.lnd OUTPUT.mesh\n";
        return 2;
    }
    LNDFile terrain;
    const auto result = terrain.Open(argv[1]);
    if (result != LNDResult::Success) {
        std::cerr << "Could not parse LND file: " << ResultToStr(result) << '\n';
        return 1;
    }
    std::ofstream output(argv[2]);
    if (!output) { std::cerr << "Cannot write output mesh\n"; return 1; }
    output << "SHADE_MESH_V1\n";
    // One triangle per line; every vertex contains x y z r g b.
    for (const auto& b : terrain.GetBlocks()) {
        for (int z=0;z<16;z++) for(int x=0;x<16;x++) {
            point(output,b,x,z);point(output,b,x+1,z);point(output,b,x,z+1);
            point(output,b,x,z+1);point(output,b,x+1,z);point(output,b,x+1,z+1);
        }
    }
    if (!output) return 1;
    std::cout << "Exported " << terrain.GetBlocks().size() << " terrain blocks\n";
    return 0;
}
