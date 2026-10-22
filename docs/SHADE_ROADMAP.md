# Shade development roadmap

Updated October 21, 2026. This is the working delivery roadmap; SHADE_BROWSER_PLAN.md retains the original goals and engine-port hypothesis.

## Product milestone

A local browser scene where one autonomous cow helps one hungry village. The player can spend Spirit for guaranteed rain, make an uncertain suggestion to the creature, or let it act independently. Learned behavior persists across sessions.

## Verified baseline

- Local Python server; no Railway dependency. Docker workflow documented but untested on this machine.
- JavaScript 2D mechanics prototype with food, worship, Spirit, trust, stamina, autonomous choices and manual save/load. Learned values persist; long-term preference learning is still rudimentary.
- Separate C++ / WASM WebGL2 procedural terrain probe.
- Original disk cabinets extracted locally. Cow preview renders 964 triangles and three embedded textures; orbit input tested in a browser.
- Main page, engine probe, health endpoint and cow-preview routes return HTTP 200.

These are separate demonstrations. The full openblack engine has not been ported, the cow is not animated, and the 3D preview is not connected to simulation state.

## 1. Establish one 3D scene — completed initial milestone

- Refactor the cow viewer into reusable rendering and asset-loading modules.
- Put the cow on a simple ground plane with a placeholder village, a clear camera, scale and ground contact.
- Support orbit, zoom, reset and pointer input without interfering with interface controls.
- Display load failures and handle lost graphics contexts.
- Keep original model data local and optional; show a useful fallback without it.

Verified October 21: original cow, ground, four placeholder houses and simple villagers render together. Orbit, zoom and reset were exercised in-browser without console errors. A separate server without the cow export rendered the placeholder and missing-asset message. Context-loss reporting is implemented but has not been fault-injected.

Acceptance: one page renders the textured cow and village together; camera controls work; missing model data produces a clear message and fallback. No server dependency beyond the local host.

## 2. Connect the playable loop — initial implementation complete

- Extract simulation state and decisions from the current 2D drawing code.
- Drive cow position and orientation from simulation state in the 3D scene.
- Connect rain, persuasion, comfort, wait, pause and new-world controls.
- Show goals, decisions, hunger, stamina and the reason a suggestion was accepted or declined.
- Preserve save/load and validate older or malformed saves.

Verified October 21: 2D and 3D share simulation.js and the same save slot. Browser tests exercised comfort, accepted persuasion, cow arrival/help reward, rain, pause and save/load. Automated checks cover refusal, movement, rewards, wait timing and malformed saves. Random wandering no longer grants food rewards. The cow translates and turns in its static pose; villagers are static and interventions currently communicate through resources and event text.

Acceptance: the three main choices produce observable, different outcomes in 3D. A creature can act without input, decline persuasion, and physically approach the village. Save/load restores its progress.

## 3. Animate the creature — next

- Inventory the cow's animation sources and identify compatible skeleton/clip data.
- Validate bone transforms, texture orientation and material handling against the source formats.
- Implement idle and walking first; then one helpful-action animation.
- Blend transitions and match walking speed to actual movement.
- Keep a static-pose fallback until each clip is verified.

Acceptance: idle, walk and help states transition reliably without broken limbs, sliding feet or gameplay stalls. Original animation support is an investigation, not a verified capability.

## 4. Make learning matter

- Replace the learned-action counter with explicit, persisted preferences driven by outcomes.
- Add repeatable scenarios for independent help, refusal, fatigue and reassurance.
- Explain decisions in plain language and show how behavior changes over repeated play.
- Tune food consumption, Spirit income, intervention cost and recovery.

Acceptance: different interaction histories produce measurably different creature decisions after save/load; persuasion remains uncertain and direct intervention remains expensive and reliable.

## 5. Expand the world carefully

- Add one village building and a small set of villagers, with readable hunger and gratitude reactions.
- Investigate original landscape loading separately; keep the simple terrain until rendering is verified.
- Add sound only after its format/loading path is understood.
- Measure frame time, memory and asset size before increasing scene complexity.

Acceptance: the village makes the core relationship easier to read and the scene remains responsive on the target machine.

## 6. Package a reliable local build

- Provide one start/stop workflow with useful diagnostics and reproducible asset preparation.
- Exercise fresh-start, missing-assets, reload, resize, graphics failure and save migration paths.
- Add targeted automated simulation/format tests and a short manual browser smoke checklist.
- Make a reviewed Git checkpoint of source changes; exclude original assets, exports, SDKs and logs.

Acceptance: a fresh source checkout plus the user's local asset folder can reproduce the demo with documented commands, without Railway.

## Separate engine-port track

Keep the full openblack browser port as a gated investigation. Audit SDL3 GPU/backend support, shader compilation, dependency builds and browser filesystem constraints. Attempt a reproducible configure/build before committing to migrating the playable prototype. The working browser loop should not wait for the full engine port.

Decision gate: integrate engine components only when a specific component is demonstrated to work in-browser and reduces maintenance or enables a required feature.

## Distribution boundary

Personal local original-asset previews remain separate from a shareable build. A public demo needs appropriately licensed replacement assets or a user-supplied asset workflow. Do not put original game data in Git, Docker images or hosted web bundles. Preserve upstream source and license attribution.

## Deferred scope

Multiplayer, accounts, cloud saves, monetization, large civilizations and generative dialogue remain deferred until the single-creature loop is enjoyable and reliable.

## Immediate work order

1. Unify cow, ground and village into one 3D scene.
2. Connect the existing simulation and save/load.
3. Investigate and implement idle/walk animation.
4. Improve learning and run a focused playtest.
