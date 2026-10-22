# openblack LND-to-WebGL2 bridge

`export_lnd.cpp` uses the existing `components/lnd` openblack parser to turn a **user-provided** Lionhead .lnd map into a simple triangular mesh stream (`SHADE_MESH_V1`). Each vertex is six space-separated floats: `x y z r g b`.

## Build with openblack's CMake project

Add an exporter executable to the existing CMake configuration or build manually against the `lnd` static target and its dependencies:

```cmake -S . -B build -DOPENBLACK_USE_VCPKG=ON
cmake --build build --target lnd
# Example manual build for a compatible toolchain:
c++ -std=c++23 -Icomponents/lnd/include web/engine-probe/export_lnd.cpp build/components/lnd/liblnd.a -o shade-export-lnd
./shade-export-lnd /path/to/authorized/landscape.lnd landscape.mesh
```

The manual linker path depends on CMake generator and platform. Terrain coordinates/heights and quad splits need reconciliation with `src/3D/LandBlock.cpp`. The current exporter is an **integration scaffold**, not yet a faithful visual reproduction. The browser-side loader has not been connected.

Do not commit or redistribute game assets or generated copies without appropriate rights.
