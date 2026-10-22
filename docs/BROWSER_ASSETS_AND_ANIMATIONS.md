# Browser asset and animation checkpoint

## Run locally

Build with `./tools/shade/wasm.ps1 -Action Build`, then run from the repository root:

    python tools/shade/serve_engine.py

Open http://localhost:8082/openblack.html?local=1 to load the prepared `.local/native-game-data` folder. The development host binds only to loopback, exposes an exact asset inventory, and disables caching to avoid mismatched JS/WASM builds. It does not change the source files. The ordinary launcher without `?local=1` offers a folder picker for a prepared Data/Scripts/Audio directory. That picker implementation still needs a separate end-to-end verification; the local HTTP import is verified.

Assets are copied into browser memory for this first pass (718 files / 433,736,829 bytes in the tested inventory), so this is a desktop development workflow. Streaming, selective level loading and memory profiling are still required before a broad online release. Original assets are not checked into Git. The hosting package should contain engine code; users can supply local assets through the picker.

## Verified on 2026-10-22

- Full WASM build and native regression build.
- Loopback health endpoint, original asset manifest, and rejection of an unlisted asset URL.
- Browser imports all 718 files, engine loads 626 packed meshes and 441 packed animations, parses Land1 and reaches the frame loop.
- Diagnostic terrain appears; dragging the focused canvas changes the view. Terrain output remains visually incorrect and lacks original materials. This is not a playable slice.
- Before enabling `-fexceptions` across engine/component compilation, an ordinary asset exception escaped as a numeric JavaScript exception. Rebuilding those libraries with exception handling allowed startup to proceed.
- Existing native warnings also appear in the browser: incomplete temple submesh, unknown A_Greek variants, an ambiguous raw texture, and unimplemented level commands for planned buildings, town flags and mist.

## Animation readiness

`python tools/shade/audit_animation.py .local/cow.json .local/cow-animation-raw.json` validates the existing experimental exports. The original cow skeleton has 48 bones; stand has 5 frames with 48 rotation/translation channels; walk has 14 frames with 29 rotation and 4 translation channels. Both contain motion. The audit checks bone compatibility, joint bounds/uniqueness, parent ordering, transform counts and finite values. An intentionally invalid joint was rejected.

This proves structural compatibility only. Creature morph timing/rotation conventions and visible skinning remain unverified. Packed AllAnims loading is a separate engine animation path; loading 441 clips does not imply that villagers or creatures are animated on screen. The existing experimental exporter sources remain separate from this checkpoint.

## Playable online slice gates

1. Correct engine terrain, original materials, and stable camera/input.
2. Original village/tree/creature models; skinning and visually verified idle/walk playback through the engine renderer.
3. One deliberately limited playable loop: explore a village, select/interact with villagers or objects, perform one god action, observe an effect, save and reload. Audit engine support before promising original-game behavior.
4. Static HTTPS engine package, user-provided asset import, progress/error handling, measured memory/frame rate, and repeatable browser smoke tests.

We have crossed compilation, renderer initialization and original asset startup. The renderer and gameplay gates remain substantial. This is several implementation milestones from a playable slice, not merely a hosting change; calendar estimates remain low-confidence until the first fully rendered island and animated character work together.


## Original terrain and scene renderer checkpoint

The browser renderer now uploads the original island BGR5A1 material array, blends the same per-triangle material channels as the native shader, applies the terrain light channel, and discards the hidden underwater triangles. The browser can draw the engine's prepared entity and tree batches, including original texture skins, rest-pose bone transforms, indexed primitives, alpha cutouts and basic blending. The shared BC1/BC2/BC3 decoder is used by native and browser uploads; packed BGRA4 and RGBA/BGRA8 skins are supported in the browser.

The development launcher provides **Village view**, which moves the engine camera to the loaded Land1 village. It is a scene inspection shortcut, not new gameplay. Manual verification showed original buildings, fields and trees in their engine positions on the original textured landscape. Startup reported 100 terrain blocks, 65 entity batches and 16 tree batches; upload, terrain and model GL checks returned zero errors. The original browser failure was caused by creating an index buffer with the vertex-buffer target, then rebinding it as an index buffer, which WebGL disallows. Index uploads now use the element-buffer target and preserve the active VAO.

Validation: full WASM Release build, native regression build, and deterministic decoder fixtures for BC1 opaque red, punch-through transparency, cropped edge output, BC2 explicit alpha and BC3 alpha. Reproduce the fixture with the local Emscripten compiler using `-std=c++23 -Isrc -I.local/wasm-build/vcpkg_installed/wasm32-emscripten/include tools/shade/test_texture_decode.cpp -o .local/test-texture-decode.js`, then run it with the local SDK Node executable.

Limits: models use rest-pose bones, capped at 64 bones per mesh; animation playback is not implemented. Terrain morphing for buildings, tree sway, water/reflections, sky rendering, bump/footprint materials, complete texture-format coverage, draw-call optimization and gameplay interaction still need work. Existing upstream script gaps remain. This is the full engine rendering its own original scene, but it is not a playable vertical slice yet.
