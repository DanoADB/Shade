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
