# OpenBlack browser parity checklist

Target: the implemented behavior of the native OpenBlack checkout in Shade, not complete Black & White campaign parity. The browser shares native asset readers, registry, scripts, camera, pathfinding and audio interfaces. A successful build alone does not establish runtime parity.

| Area | Browser status | Remaining acceptance work |
|---|---|---|
| Original data and Land1 startup | Verified | Other supported levels and clean-profile startup |
| Terrain materials | Partial | Bump maps, shore alpha, building footprints |
| Village, trees, original hand | Verified basic rendering | Native material/lighting match, terrain deformation and tree sway |
| Packed villager animations | Implemented | Broader clip coverage and walking verification; native currently uses rest poses |
| Sky | Original dome and nine-layer shader implemented; day verified | Verify time/alignment controls and other scenes |
| Ocean | Missing | Original water texture, reflection framebuffer and reflected scene |
| Native debug UI | Disabled | Browser renderer backend for existing menus and inspectors |
| Offscreen render targets | Stub | Reflection, footprints, mesh viewer and texture viewer |
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
