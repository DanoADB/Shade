# OpenBlack / Shade functionality audit

Checked 2026-10-23 against this repository's native OpenBlack code and Shade browser build. This is a source review plus targeted runtime checks, not certification of every original Black & White feature. The existing [browser parity checklist](OPENBLACK_BROWSER_PARITY.md) records earlier native/browser tests. No percentage of overall parity is defensible yet.

Shade's intended differences remain: the player is unseen, villagers worship the creature, and the player teaches through influence. Restoring the visible hand or direct control in this scenario would contradict that design. `?native=1` remains the engine comparison route.

| Function | Native OpenBlack baseline in this checkout | Shade browser behavior | Gap / verification |
|---|---|---|---|
| Original assets, islands | Shared L3D, landscape, pack, animation and script readers; original island loading | Same readers, automatic hosted assets, original village/tree/creature art | Earlier island checks cover Land1–5, LandT and demo2; clean device and matched-camera coverage incomplete |
| Terrain, ocean, sky, lighting | Native rendering passes | WebGL implementations, reflections, decals, compressed textures; extra drought atmosphere | Visual equivalence and device performance still need systematic measurements |
| Creature creation / choice | Species and appearance mesh lookup; creature archetype creation | Live Cow / Rhino / Sheep selector with separate original CBN banks | First three choices only; other species, alignment and body variants pending |
| Creature animation | Original CBN parser/assets exist; archetype alone does not implement creature behavior | Stand, walk, drink, work, sleep/wake, casting and celebration selected by drought action; interpolation, short blends and distance-based gait | All 11 requested clips sampled on three 48-bone rigs; browser swaps checked. Tiger/Chimp rig mapping, foot IK, hand-to-prop attachment and full visual coverage of every clip remain |
| Creature intelligence | `CreatureMind` is a dummy struct; learning/desire/action CHL calls include stubs | Drought autonomy, energy/confidence, nudges, reinforcement and learned action weights | Slice rules are an addition, not original creature AI parity; object-specific learning, persistent personality and broader actions missing |
| Village agency / economy | Town lookup, housing, living-action and pathfinding systems; living states include unimplemented handlers | Three original villagers reprioritize water/crop/forest work and carry props | Jobs operate on scenario meters; no full household needs, inventory, construction, population or production chains |
| Movement / interaction | Shared camera, hand/dynamics/pathfinding interfaces | Desktop/touch camera, scene inspection, creature follow; drought actors use clearance- and slope-aware local routes | Local obstacle navigation implemented; dynamic replanning, agent avoidance and moving prop grip remain |
| Miracles / belief | Script interfaces exist, with many unfinished gameplay commands | Faith/confidence-gated rain and creature casting; water delivery earns faith | Limited miracle set and aggregate belief; no complete worship economy, influence territory or original spell system |
| Waterworks / environment | Shared landscape and world entities | Discover spring, build persistent-in-run watercourse, change survival outcome; surface meshes and weather | No excavation/fluid physics; drought meters are not per-field or per-tree simulation |
| Save / restore | `SAVE_GAME_IN_SLOT` is an explicit stub | Earlier village task checkpoint exists; versioned drought save restores god, workers, learning and in-flight actions, paused on reload | Single local browser/site slot; full native world and cloud saves remain absent |
| Campaign / quests | Script VM runs; numerous CHL APIs remain unimplemented | Story islands can load; one custom drought scenario has win/loss | Loading an island is not playable campaign progression; quest-by-quest native command tracing required |
| Citadel / audio / lifecycle | Native systems and debug tools | Citadel enter/exit, audio playback path, pause/hidden-page suspension and context-loss recovery previously checked | Audible/spatial quality, wider Citadel interactions and device lifecycle coverage incomplete |
| Online delivery | Native executable | Packaged WASM + automatic asset download; local static server / Railway | Hosted artifact verification is separate from pushing; physical mobile and slow-network testing pending. Online here means browser delivery, not multiplayer |

## Ordered development backlog

Priority describes the next playable Shade milestone, not the amount of unfinished original-game functionality.

| Order | Priority | Work | Completion gate |
|---|---|---|---|
| 1 | P0 | God choice and action animation foundation — implemented in this batch | Swap Cow → Rhino → Sheep → Cow while paused and active without resetting water, workers, time or learning; matching rig, finite poses and no new browser errors |
| 2 | P0 | Versioned drought save / resume, including god and learning — implemented | Reload halfway through care, rest and channel construction; restore without duplicated gifts/workers/props; reject corrupt saves safely; explicit new-game reset |
| 3 | P0 | Obstacle-aware creature and worker navigation — implemented | Walk around village buildings/trees, arrive at water sites, stop without sliding, and recover from unreachable destinations; repeat for all three gods |
| 4 | P1 | Physical action readability | Bone-attached bucket, contact-aware feet, visible delivery/work effects and expressions; walk/drink/sleep/cast/celebrate checked for every supported rig |
| 5 | P1 | Village needs and resource ownership | Household water/food, carried inventories, rest and construction jobs; changing priorities alters actual production and visible settlement state |
| 6 | P1 | Expand creature learning and belief | Remember preferences and consequences across sessions; target-specific choices; villagers react to witnessed creature acts; distinct species traits with measurable tradeoffs |
| 7 | P1 | Complete drought release gate | Guided victory and unattended failure, touch-only completion, save/reload completion, no console/GL errors; profile on real phone and desktop and establish frame/memory/download budgets |
| 8 | P2 | Broader native parity, then more scenarios | Trace commands reached by a selected native quest, implement shared missing calls with native/browser fixtures, and complete that quest end-to-end before advancing; expand species, miracles and Citadel interactions incrementally |

## Evidence and limits

- `src/Creature/CreatureMind.h`: placeholder intelligence. `src/ECS/Archetypes/CreatureArchetype.cpp`: species-based creation. `src/3D/CreatureBody.cpp`: original species/appearance lookup.
- `src/CHLApi.cpp`: `CreatureLearnEverything`, `CreatureSetKnowsAction`, `CreatureDoAction`, and `SaveGameInSlot` explicitly log unimplemented; these are shared baseline gaps, not WebGL failures.
- `src/ECS/Systems/Implementations/LivingActionSystem.cpp`: unimplemented entry/state/exit/save/load handler diagnostics. Existing working handlers must be assessed individually rather than declaring the whole system absent.
- `web/drought-model.js`, `web/drought.js`, `src/BrowserDrought.cpp`: current Shade rules, UI, actor movement and animation bridge.
- `tools/shade/test_creature_animation.cpp`: sparse channels, interpolation, hierarchy, looping, one-shots and root travel removal. `test_original_creature_animation.cpp`: all selected action clips sampled against local original Cow/Rhino/Sheep meshes; finite matrices, motion and cow pose bounds. Tiger and Chimp were numerically sampled. Tiger failed the sustained visual rig-mapping check; Chimp has not passed full visual validation. Both remain excluded from the selector until their mappings are validated.
- `web/tests/drought-model.test.cjs` and `drought-ui.test.cjs`: timing, learning, outcomes, workers, pause, suspension, follow and swap UI failure rollback. These are not substitutes for terrain collision, visual quality or actual-device tests.

The first playable loop already exists. The next goal is to make it durable and physically convincing before expanding toward campaign breadth.
