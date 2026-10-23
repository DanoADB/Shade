# The Long Dry: first playable slice

The default browser engine launch now starts Shade's three-minute drought scenario. The player has no visible hand or direct resource-giving controls; villagers attribute help to the original cow creature. The creature weighs compassion, curiosity, rest, energy and learned habits. Praise and unease reinforce or discourage its last completed action. Nudges affect the next decision rather than canceling current work.

## Play

- Open `/` and choose **Play The Long Dry**, or open `/engine/openblack.html`.
- Keep water at least 25, crops at least 40 and forest at least 35 when the 180-second dry season ends.
- Encourage discovery to find a spring. After two water deliveries and 28 faith, compassion can lead the creature to open a shared channel.
- A rain dream becomes available at 44 faith, 48 confidence and 45 energy. Rain helps recovery but costs the creature energy.
- Praise only responds once per completed action, during a short window. Repeated nudges have a cooldown; a stronger bond makes them more persuasive.
- **Find creature** switches to a close view; **Village view** returns to the wider framing. Drag/pinch and the existing touch controls still work.
- Pause, P, switching away from the page, and Citadel entry suspend scenario time. Begin again starts a fresh run. Progress is not persisted across reloads.

Three original villagers work autonomously as water gatherer, farmer and forester. They complete work cycles that affect scenario resources and reprioritize when a resource becomes scarce. Their original models move across terrain and use original villager walking clips; water deliveries trigger gratitude animations. Villagers credit the creature in the scenario's event history.

The original C_Cow_Boned model (48 bones) is used, with an original bucket during water-carrying trips. Creature locomotion currently uses ground-following translation, turning and body motion; dedicated creature skeletal walk/work animation is not implemented. Worker paths are simple terrain-following routes, not full settlement navigation. Water, crops, forest, faith and learning are slice-level simulation values, not a replacement for native OpenBlack's economy/AI. The channel and spring are functional scenario states; there is no excavated terrain mesh or fluid simulation yet.

## Presentation

The slice adds responsive woodland-and-brass UI, a new landing page, warmer terrain/object colour, distance haze and a cooler wet atmosphere during rain. Original terrain, buildings, foliage and creature art remain in use. Rain streaks are a lightweight overlay that respects reduced-motion preferences. Native comparison mode keeps its original look and debug menus.

## Regression and comparison

- `node web/tests/drought-model.test.cjs`: deterministic timing, cooldown, learning, autonomous worker reprioritization, channel/rain outcomes, failure, terminal states and guided victory versus unattended failure.
- `node web/tests/drought-ui.test.cjs`: startup gates, pause, hidden-page/Citadel suspension, restart and map isolation.
- Existing launch, village task and mobile controls tests remain applicable.
- WASM and Windows builds passed. Browser playtesting reached the spring/channel and a completed victory on both balances. The tougher browser run ended with water 53, crops 65 and forest 68 after three creature deliveries and the community channel. The original cow and three original worker villagers loaded without console errors. A guided-win versus unattended-loss model regression also passed. A phone-sized viewport was checked with expanded and collapsed UI; physical-device performance remains unverified.

`?native=1` retains isolated OpenBlack comparison mode. `?scenario=village` retains the earlier direct wood/food task. The hosted package includes both drought scripts; the Docker image checks that they exist. Experimental asset-exporter changes are outside this feature.

## Visible waterworks and work props

The scenario now has an original-art old well, a ground-following pool, an upstream pool revealed by exploration, and a progressively built earth-and-water channel. Completed watercourses show moving flow highlights. These are surface meshes; terrain excavation and fluid physics remain outside this slice. Villagers carry original buckets, food or wood props on their return trips. Inspectable scene labels show water sites and workers and can move the camera to them. Anchors remain current while the scenario is paused. Restart clears generated props and surface geometry.

The standalone drought geometry check covers discovery visibility, partial/completed construction, terrain fit, out-of-range progress and duplicate path points. Model/UI regressions passed. Browser startup rendered 144 surface vertices with GL error zero and displayed the original well and inspectable labels without new console errors. Original creature skeletal animation work is the next active batch.
