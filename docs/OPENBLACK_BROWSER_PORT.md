# Openblack browser port: active delivery plan

Updated October 22, 2026. This supersedes the browser-native-first sequencing in SHADE_ROADMAP.md. Shade will be an homage and honorary remaster built after the openblack engine runs in the browser. The existing JavaScript scene is a reference and asset diagnostic, not the engine implementation.

## Order of work

1. Establish a native openblack baseline using the supplied original game data. Record startup, scene, camera/hand controls, terrain, water/sky, models, animations, sound and simulation behavior. Distinguish missing upstream behavior from browser regressions.
2. Configure and compile the full C++ engine for WebAssembly. Separate build-host shader tools from target libraries. Track each unsupported dependency and platform API.
3. Implement the browser renderer behind the engine interfaces. Choose a backend only after a representative engine rendering test; the procedural WebGL2 probe does not validate SDL GPU support. Cover terrain/material blending, depth/blend passes, water, sky, meshes, skinning, UI and render targets.
4. Adapt filesystem, main loop, input, audio and save storage. Load user-selected assets locally in the browser; keep original data outside hosted bundles. Provide progress and actionable errors.
5. Reach parity with the measured native baseline on a bounded test island and then the broader supported game scenes. Profile memory, startup, frame time and asset loading. Test persistence and graphics recovery.
6. Publish a reproducible HTTPS browser build with source/license notices, compatibility guidance and bring-your-own-data setup. Verify it from a clean browser profile and another machine.
7. Begin Shade gameplay changes only after the browser engine acceptance gate passes.

## Acceptance gate before Shade gameplay changes

A fresh browser session can load the user's data, start a supported island, render the scene and animated inhabitants, use the camera/hand interactions supported by the native baseline, run the same simulation/script behavior, play audio after user interaction, and save/reload without fatal errors. Document known upstream omissions rather than implying the original campaign is complete.

## Current state

- Original asset extraction and the separate JavaScript gameplay/reference scene work locally.
- The C++ asset readers and graphics probe build with Emscripten; this is not a full-engine browser build.
- Native configure/build succeeded after enabling SDL3 Vulkan support on Windows. Land1 renders terrain, water/reflections, sky, vegetation and the hand cursor; camera drag was verified. See OPENBLACK_NATIVE_BASELINE.md. Full-engine WebAssembly Release configure and link now pass. The generated engine executes in Node (--help) and initializes SDL in a browser, then stops because its Vulkan renderer is unsupported. Native host shader generation also passes independently.
- Earlier experimental cow animation export is preserved but is no longer the main implementation path.

## Local build and data

Source assets: C:\Users\danod\OneDrive\Desktop\B&W.
Extracted file groups: .local/game-data-base.
The native preparation script tools/shade/prepare_native_data.py assembles local Data/Scripts/Audio trees without executing installers. The mapping is provisional until startup and assets are verified; extracted files have not been patched to a later game version.
Build output and logs belong under .local; proprietary data and generated copies are excluded from Git and Docker contexts.
