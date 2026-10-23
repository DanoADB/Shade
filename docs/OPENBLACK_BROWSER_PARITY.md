# OpenBlack browser parity checklist

Target: the implemented behavior of the native OpenBlack checkout in Shade, not complete Black & White campaign parity. The browser shares native asset readers, registry, scripts, camera, pathfinding and audio interfaces. A successful build alone does not establish runtime parity.

| Area | Browser status | Remaining acceptance work |
|---|---|---|
| Original data and story islands | Land1–Land5 and LandT loaded in browser; isolated native startup available | Demo2, clean-profile startup and broader scene comparisons |
| Terrain materials | Native bump/detail, shore alpha and footprint passes implemented | Broader native pixel/scene comparison |
| Village, trees, original hand | Original models, native lighting, terrain deformation and sway implemented | Wider model/material coverage and native visual comparison |
| Packed villager animations | Implemented | Broader clip coverage and walking verification; native currently uses rest poses |
| Sky | Original dome and nine-layer shader implemented; day/night verified | Broader alignment and scene comparisons |
| Ocean | Original water textures and reflected-scene pass implemented; Land2 coast verified | Native visual comparison across time/alignment and views |
| Native debug UI and Citadel | Native menus, mesh preview, image tools and Citadel enter/exit verified | Wider inspector, room navigation and keyboard coverage |
| Offscreen render targets | Reflection, footprints and mesh preview implemented | More scene/format and resize coverage |
| Lines and overlays | Scene/mesh bounding boxes, paths and streams verified | Compare remaining overlays against native |
| Sprites | Native billboard/additive pass; Citadel lamp glow disappears/reappears with native Sprites toggle | Bookmark cases and native visual comparison |
| Audio | Original cow bank reached Playing with advancing progress through native UI | Audible/spatial quality and more devices/tracks |
| Simulation and script VM | Shared code runs | Compare supported native commands and timing on multiple scenes |
| Input | Basic camera/hand and touch verified | Native hotkeys, focus/capture, full interaction comparison |
| Storage | Shade task checkpoint works; native SAVE_GAME_IN_SLOT remains a source stub | Further native-supported file-write paths; no full world-save claim |
| Browser lifecycle | Quit and explicit graphics recovery verified; resize implemented | Memory/frame-time, clean profile and cross-device tests |
| Deployment | Packaged engine pushed | Verify latest hosted artifacts and another device |

Existing native gaps include unimplemented script commands, incomplete creature/gameplay behavior and unverified original campaign progression. These must remain distinguished from browser-specific omissions. Shade wood/food tasks are additions and are not evidence of native parity.

The earlier bring-your-own-assets deployment plan is superseded by the user's hosted automatic asset package. Experimental exporter edits remain outside the browser port changes.

## UI backend checkpoint

Vendored the matching Dear ImGui 1.92.9b OpenGL backend from the installed dependency source, preserving its headers. Native SDL GPU builds keep their original backend. Browser tests opened the native World menu, changed to night, and loaded Land2 from Load Island. Original Land2 terrain, architecture and sky rendered without console errors. Deferred scene selection until native input capture settles. Loading another island disables the Land1 task and its camera shortcuts so saved task state cannot leak across maps. WASM/native builds and input/storage regression checks passed. Offscreen mesh/texture inspectors and capture are still incomplete.

## Ocean checkpoint

Replaced empty browser framebuffer handles with sampled RGBA8 targets and optional depth/stencil textures. The main scene now renders the engine reflected camera into the ocean target before drawing the original six-index sea plane. Water uses the same diffuse/alpha/horizon mixing as the native shader, with OpenGL texture Y conventions. R8 texture inputs now upload, and terrain retains its shoreline alpha over the water. Land1 startup and Land2 switching/coast rendering passed; framebuffer and water GL checks returned zero and no console errors appeared. Reflection visuals still need broader native comparison. Render targets increase graphics memory use; profiling remains an acceptance gap.

## Terrain and vegetation checkpoint

Ported the native terrain bump/detail shading, original ground decals, height-map deformation for marked buildings/fields, tree anchoring/sway and native object/vegetation day-night lighting equations. Ground decals use an orthographic RGBA target with OpenGL texture orientation. Browser Land1 rendered 1,485 decal instances into a 3,584 by 3,328 target with GL error zero. Original building surrounds and conforming crop fields are visible; the World menu dims terrain and buildings at night. No console errors appeared. Larger scene/material comparisons and performance profiling remain required.

## Inspectors, overlays and capture checkpoint

Implemented offscreen DrawMesh with native submesh filtering and caller-provided bone palettes, plus bounding-box drawing. ImGui framebuffer previews use OpenGL UV orientation. The native MeshPack Viewer displayed the original dummy mesh texture and red box, while scene bounding boxes, footpaths and streams rendered through native view toggles. Added the native billboard sprite/tint pass; specific sprite visuals remain unverified. Capture now downloads a PNG from the completed engine canvas; the resulting screenshot.png was opened and visually checked. HTML task controls are outside the engine canvas and are not part of that native capture. WASM and native regression builds passed; no browser console errors appeared during these checks.

## Audio and lifecycle checkpoint

The native Audio Player selected bcow.sad, decoded a sound, and displayed Playing with advancing progress (17 percent observed) without console errors. This verifies the running playback path, not an independent audible-quality assessment. SDK OpenAL already resumes its Web Audio context on interaction. Browser Quit now stops task polling/held controls before engine services are destroyed. Graphics context loss pauses the engine and exposes an explicit reload control; restoration rebuilds through a page restart rather than reusing invalid GPU handles. The native Debug/View menu includes a recovery diagnostic. Simulated loss/restoration, reload and native Quit passed in-browser; lifecycle/input tests and WASM/native builds passed.

## Native render-state comparison

A native Vulkan Land4 launch completed three frames and produced a reference PNG. That comparison exposed the browser's missing terrain face culling: back faces obscured the native below-ridge starting view. Main terrain now culls back faces, reflection terrain culls front faces, and footprints follow native culling. Terrain/object alpha blending and debug-line depth/blend states now match native pipeline settings. Screenshot capture excludes the native debug UI, as the Vulkan capture path does. The corrected browser Land4 view exposes the lake and reflected trees without GL errors; native and browser views still require matched-camera comparison. This test also exposed a rapid menu-click/camera-input leak, tracked as the next fix.

## Native menu focus checkpoint

Browser event ownership now uses the current pointer/touch position and retains press ownership through release. Synthetic HTML camera controls bypass UI capture. Transient input is cleared each frame, while camera action/interpolation updates pause when native UI owns input. A trace confirmed mouse menu presses were captured; continuing camera-controller updates under GUI focus caused the remaining visible movement. Rapid Capture-menu use now preserves the Land4 view, and engine-only capture still downloads correctly. World drag responds, but low-view Land4 drag behavior still needs matched native-input comparison. Temporary input traces were removed. Native/WASM builds and existing input regressions passed.

## Citadel and skeleton checkpoint

Replaced the 64-matrix vertex uniform array with an RGBA32F texture palette. This avoids the minimum WebGL2 vertex-uniform budget and removes the arbitrary 64-bone skip; device texture height remains an explicit checked limit. Land1 uploaded a 22-bone palette with GL error zero. The Citadel rendering system has no per-instance animation records, so the browser now uses its rest pose rather than indexing missing metadata. Entering the original Citadel rendered its textured hall, columns, door and blue foreground surface; exiting restored the island without console errors. The Shade task and camera presets suspend inside the Citadel and remain suspended on other islands. Regression checks cover both transitions. WASM and Windows builds passed. Broader skeleton/animation and native Citadel visual comparisons remain pending.

## Clean native launch and island coverage

Browser smoke tests additionally loaded Land5 and LandT through the original menu without console errors. Added `?native=1` for an OpenBlack startup without Shade task polling, save restoration, selection or camera presets. `?level=Land4.txt` also selects native mode; levels are restricted to the supported story/demo allowlist and invalid input falls back to Land1. This makes startup comparisons reproducible. Regression tests cover level validation and isolation of task saves, updates, polling and Citadel exit. A clean Land4 startup settles to a ground-close browser view. A longer Windows run (18,000 frames) also settled to a ground-close view, so this is shared behavior rather than evidence of a browser-only camera regression. Exact timing/input and image comparison remains open. The public Railway package was still reporting source revision c535fd22 during this check, so latest deployment is not yet confirmed.


## Matched-duration and sprite follow-up

A Windows Land4 run exited successfully after 18,000 frames and produced `.local/native-land4-long.png`; it shows the same ground-close class of view as the browser after settling. The earlier 600-frame screenshot was too early to establish a discrepancy. In the original Citadel, turning Sprites off removed the bright lamp glow beside the main door; turning it back on restored the glow. No browser console errors appeared. Local routes `/`, `/engine-probe/`, `/healthz` and `/engine/openblack.html?native=1` returned HTTP 200. Railway had advanced to the Citadel package (manifest source 5bf8001e) but clean-launch deployment was still pending at the last check. Native source stubs, exact input/render comparisons, more audio/devices and performance remain unresolved parity gates.

## Compressed texture parity and memory

The WebGL renderer now uses original BC1/BC2/BC3 blocks when WEBGL_compressed_texture_s3tc is available, matching the native backend's compressed-upload preference. Unsupported devices and failed uploads retain the shared software decoder. DDS mip payloads are restricted to the base level. `?textures=decoded` forces the fallback for comparison. Browser Land1 startup passed in both modes with no warnings/errors: 110 compressed uploads occupied 5,406,720 bytes instead of 28,835,840 decoded bytes (texture payload only; not total GPU memory). Both views rendered the original coast, ocean, reflection and vegetation. Existing decoder fixtures, launch, gameplay and mobile regressions passed; WASM built and was repackaged. Physical mobile-device testing remains open.

## Single-channel texture parity

R8 source textures now upload as GL_R8 rather than expanded grayscale RGBA. This matches native single-channel sampling for original height, terrain bump/detail and ocean alpha maps, avoids temporary expansion, and preserves byte-aligned upload handling for odd widths. Browser Land1 reported 69 R8 uploads totaling 4,180,357 bytes instead of 16,721,428 expanded bytes. Original terrain, shore, water and reflection rendered without warnings/errors. WASM and launch/gameplay/mobile regression checks passed; package refreshed. Keyboard P started the shared simulation and the native overlay showed advancing turns; arrow-right moved the camera. Ctrl+1 created the original animated bookmark sprite. Bookmark flight completion remains under verification.
