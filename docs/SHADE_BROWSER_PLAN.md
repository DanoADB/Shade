# Shade — Browser-First Engineering Plan

## Source baseline
- Repository: DanoADB/Shade
- Development branch: `shade-development`, created from `master`.
- Upstream engine: openblack (GPL-3.0).
- Original Black & White data files are proprietary and must **not** be committed or distributed without appropriate rights.

## Goal
A browser-based creature-centered simulation. The player is the unseen Creator; a physically present autonomous animal is perceived as a god by villagers. Worship generates Spirit, which the player spends on direct interventions (expensive) or nudges/persuasion (cheaper, uncertain). The creature can act independently, using physical effort instead of player Spirit.

## Architectural hypothesis — not yet validated
Compile existing C++ engine to WebAssembly with the repository's `emscripten-vcpkg` / `emscripten-vcpkg-release` CMake presets. Serve build artifacts as static browser assets; deploy static host and optional persistence API to Railway. The existing preset proves a toolchain configuration exists, **not** that a playable browser build works.

## Gate 0 — establish the baseline
1. Keep upstream `master` pristine. Develop on `shade-development` with small commits.
2. Inspect current graphics backend, window/input library, filesystem assumptions, resource packaging and dependencies for browser support.
3. Attempt reproducible Emscripten release configure and build, recording exact versions, errors, fixes, output size and command lines.
4. Bring up a simple browser page that loads the compiled application and reports errors visibly.
5. Demonstrate a rendered scene with camera controls and reliable input. **Do not claim success before testing in a real browser.**

## Gate 1 — assets and distribution
- Identify the minimum assets needed for a demo.
- Do not place copyrighted original game files in Git, Docker images, public object storage or client bundles.
- For personal testing, investigate user-supplied asset loading. For a public hosted release, use original/appropriately licensed replacement content.
- Preserve GPL license notices and meet source-distribution obligations for derivative binaries.

## Gate 2 — one playable loop
- One creature, one village, a food shortage.
- Creature: independent actions, goals, needs, stamina, learned preferences and outcomes observable in a debug overlay.
- Creator: anonymous physical nudge and Spirit-funded intervention.
- Villagers: worship and belief attribution based on observed events, not omniscient truth.
- Spirit: worship-based income and sharply differentiated costs; values are tuning parameters.
- Save/load enough state to test whether learned behavior survives sessions.

## Design acceptance test
The player can either spend substantial Spirit for a guaranteed immediate intervention, persuade the creature with lower Spirit and uncertain outcome, or wait for autonomous action. These choices must produce different consequences. The creature must be able to reject a request, but rejection should remain legible and interesting.

## Avoid premature scope
No multiplayer, full civilization simulation, advanced generative AI, custom account systems or monetization until the browser build and core relationship loop are playable.

## Railway target
- Initially deploy a static web server serving WASM, JS, HTML and **authorized** assets.
- Add backend services only when saves/accounts or optional AI truly require them.
- Validate COOP/COEP and other browser headers only if build settings require shared memory/threads; validate WebGL/WebGPU backend support experimentally.

## Immediate engineering deliverables
1. Browser compatibility audit with specific blocking files and libraries.
2. Reproducible Emscripten build script or CI workflow.
3. Browser smoke test that visibly renders and handles input.
4. Railway deployment config only after the smoke test works locally.
