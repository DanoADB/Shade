# OpenBlack browser parity checklist

Target: the implemented behavior of the native OpenBlack checkout in Shade, not complete Black & White campaign parity. The browser shares native asset readers, registry, scripts, camera, pathfinding and audio interfaces. A successful build alone does not establish runtime parity.

| Area | Browser status | Remaining acceptance work |
|---|---|---|
| Original data and Land1 startup | Land1 and native-menu switch to Land2 verified | Other supported levels and clean-profile startup |
| Terrain materials | Native bump/detail, shore alpha and footprint passes implemented | Broader native pixel/scene comparison |
| Village, trees, original hand | Original models, native lighting, terrain deformation and sway implemented | Wider model/material coverage and native visual comparison |
| Packed villager animations | Implemented | Broader clip coverage and walking verification; native currently uses rest poses |
| Sky | Original dome and nine-layer shader implemented; day/night verified | Broader alignment and scene comparisons |
| Ocean | Original water textures and reflected-scene pass implemented; Land2 coast verified | Native visual comparison across time/alignment and views |
| Native debug UI | Native menus restored with Dear ImGui WebGL2 backend | Offscreen inspector rendering still needs render targets |
| Offscreen render targets | RGBA8/depth-stencil targets implemented; reflection running | Footprint and inspector draw passes |
| Lines and overlays | Missing | Bounding boxes, paths, streams and debug entities |
| Sprites | Missing | Native sprite pass and blend behavior |
| Audio | Compiles | Browser gesture unlock, real playback and spatial controls |
| Simulation and script VM | Shared code runs | Compare supported native commands and timing on multiple scenes |
| Input | Basic camera/hand and touch verified | Native hotkeys, focus/capture, full interaction comparison |
| Storage | Shade task checkpoint works | Native-supported persistence behavior; this is not full world saving |
| Browser lifecycle | Partial | Context recovery, resize, memory/frame-time and clean launch tests |
| Deployment | Packaged engine pushed | Verify latest hosted artifacts and another device |

Existing native gaps include unimplemented script commands, incomplete creature/gameplay behavior and unverified original campaign progression. These must remain distinguished from browser-specific omissions. Shade wood/food tasks are additions and are not evidence of native parity.

The earlier bring-your-own-assets deployment plan is superseded by the user's hosted automatic asset package. Experimental exporter edits remain outside the browser port changes.

## UI backend checkpoint

Vendored the matching Dear ImGui 1.92.9b OpenGL backend from the installed dependency source, preserving its headers. Native SDL GPU builds keep their original backend. Browser tests opened the native World menu, changed to night, and loaded Land2 from Load Island. Original Land2 terrain, architecture and sky rendered without console errors. Deferred scene selection until native input capture settles. Loading another island disables the Land1 task and its camera shortcuts so saved task state cannot leak across maps. WASM/native builds and input/storage regression checks passed. Offscreen mesh/texture inspectors and capture are still incomplete.

## Ocean checkpoint

Replaced empty browser framebuffer handles with sampled RGBA8 targets and optional depth/stencil textures. The main scene now renders the engine reflected camera into the ocean target before drawing the original six-index sea plane. Water uses the same diffuse/alpha/horizon mixing as the native shader, with OpenGL texture Y conventions. R8 texture inputs now upload, and terrain retains its shoreline alpha over the water. Land1 startup and Land2 switching/coast rendering passed; framebuffer and water GL checks returned zero and no console errors appeared. Reflection visuals still need broader native comparison. Render targets increase graphics memory use; profiling remains an acceptance gap.

## Terrain and vegetation checkpoint

Ported the native terrain bump/detail shading, original ground decals, height-map deformation for marked buildings/fields, tree anchoring/sway and native object/vegetation day-night lighting equations. Ground decals use an orthographic RGBA target with OpenGL texture orientation. Browser Land1 rendered 1,485 decal instances into a 3,584 by 3,328 target with GL error zero. Original building surrounds and conforming crop fields are visible; the World menu dims terrain and buildings at night. No console errors appeared. Larger scene/material comparisons and performance profiling remain required.
