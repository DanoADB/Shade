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

Original Cow (48 bones), Rhino (48) and Sheep (48) models now have matching CBN skeletal animation banks. The Their god selector changes model and motion bank while preserving the current village, time, action and learned preferences. An original bucket appears during water-carrying trips. Creature and worker paths now use clearance- and slope-aware terrain routing; see the navigation checkpoint below. Water, crops, forest, faith and learning are slice-level simulation values, not a replacement for native OpenBlack's economy/AI. The channel and spring are functional scenario states; there is no excavated terrain mesh or fluid simulation yet.

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

The standalone drought geometry check covers discovery visibility, partial/completed construction, terrain fit, out-of-range progress and duplicate path points. Model/UI regressions passed. Browser startup rendered 144 surface vertices with GL error zero and displayed the original well and inspectable labels without new console errors. Original creature skeletal animation and god selection are now implemented; see the checkpoint below.

## Creature motion and god selection

Cow, Rhino and Sheep load their own original animation banks (160, 160 and 160 unique named clips). Eleven action clips cover standing, walking, drinking, channel work, sleep/wake, rain casting and celebration. Sparse channels use quaternion interpolation, short transitions and in-place root travel; gait follows distance travelled. Ground contact correction offsets the whole animated body; it is not per-foot terrain IK. All gods are normalized to an eleven-metre standing height for scenario presentation. The bucket still needs a bone attachment.

Use **Their god** to swap while active or paused, and **Follow creature** to keep it framed. Switching does not reset village resources, workers, scenario time or learning; failure leaves the previous selection active. Begin again retains the chosen god but resets the scenario. Reload restores the saved run and god, paused until Resume. Species currently share scenario rules and do not have separate personalities or balance.

Synthetic animation tests and original-asset sampling cover all three rigs. Browser standing poses and live swaps were checked; not every action on every rig has received visual acceptance. See [the functionality audit](FUNCTIONALITY_AUDIT.md) for evidence, limits and ordered development gates.

## Save and resume

The drought scenario autosaves every two seconds and after teaching, god changes, pause/follow changes, and page hiding/leaving. **Save game** writes an immediate checkpoint. Reload automatically restores it, paused; press **Resume** to continue. Time away does not advance the simulation. **Begin again** replaces the saved run with a fresh season while retaining the selected god.

The versioned checkpoint includes god choice, follow view, deterministic random state, village water/crops/forest, faith, energy, confidence, bond, spring/channel/rain state, deliveries, in-flight action and progress, reinforcement/cooldown windows, learned habits, and each worker's job/progress/completed cycles. Restored delivery counters do not replay the village's gift celebration. Scene props and waterworks rebuild from this state instead of serializing engine entities.

Saves are local to this browser and site address; localhost and Railway have separate saves. This is one drought save slot, not a full native world or cross-device/cloud save. Corrupt or unsupported saves are retained without automatic overwrite; Save game or Begin again explicitly replaces them. Storage failures display a notice and leave the current session playable.

`node web/tests/drought-save.test.cjs` checks deterministic continuation at every quarter-second of a guided run, action/worker completion boundaries, all gods, restored learning and progress, paused startup, fresh-season replacement, invalid data, storage failures, failed god loading, safe history rendering and page-hide persistence.

## Safe navigation

The creature and three scenario workers now follow deterministic A* routes instead of straight lines. A two-metre grid covers the local settlement. Buildings use existing Fixed obstacle circles; trees protect their scaled trunks rather than blocking their entire swaying canopy. The creature has 3.2 metres of clearance, villagers 0.65 metres. Routes reject water, steep slopes (about 31 degrees for the creature, 33 for workers), diagonal corner cutting and unsafe shortcuts. Destinations inside obstacles resolve to reachable approach points.

Ground heights interpolate the same split triangles as the rendered landscape. The native height lookup is cell-stepped; using it directly incorrectly treated cell edges as cliffs and buried actors in slopes. Route interpolation follows path distance, then places the actor on the rendered terrain. The channel uses the reverse spring route, so its work path stays navigable too.

If a route is unavailable, the actor stays at a safe location and the UI explains the wait. Blocked trips cannot complete deliveries or worker rewards. Workers at home can choose a reachable alternative job. In-flight blocked actions wait without discarding saved progress. Routes are rebuilt on startup/new season; the version-one save continues to store logical progress rather than renderer entities. All three gods use the same conservative clearance profile.

The island browser test found all twelve route flags available (two creature trips, channel, and nine worker/job combinations) around 248 original obstacles. The spring path required seven waypoints. The standalone terrain-route fixture covers circle clearance, both landscape triangle splits, detours, inaccessible cliffs/water, actor sizes, blocked starts, reachable alternative approaches and path sampling. Drought model/save/UI tests cover blocked rewards and restored stalled progress. This is local static-obstacle navigation, not moving-agent avoidance, per-foot IK, a full-island navmesh or dynamic construction replanning. Visual limbs and foliage can extend beyond the navigation body.

## Visible interaction hand

The player's hand is visible again; hiding the player from the villagers does not hide the interaction cursor. Drought presentation/animation and native-menu suppression now use a separate scene flag from hand visibility. In **Hand tools**, use Find tree / Find food (or tap the world), then Pick up wood / Pick up food. Select a building to deliver resources, a hungry villager to feed food, or Return resource to cancel. Finding/selecting an object releases creature-follow so the camera stays on your selection. Mouse drags, pinches and cancelled touches do not select objects.

These controls restore the existing original-art wood/food bundle interactions. They do not yet provide arbitrary physical grabbing of villagers, buildings or the creature. The drought save preserves the scenario; held resources return to their source on reload, and native inventory changes through these tools are not part of that checkpoint. Villagers' drought faith remains tied to the creature's scenario actions.

Browser acceptance: the original orange hand follows a click onto terrain; wood pickup and delivery to a building, food pickup and return all reached the expected live states. The food offering is visible above the hand. No new browser console warnings/errors appeared during this check.

## Ctrl + left mouse resource grab

On desktop, hold Ctrl and press the left mouse button over a tree or field to immediately pick up wood or food. Move the pointer while holding the chord; release either Ctrl or the left button to release the bundle. Releasing over a building delivers it; food released over a hungry villager feeds them. Otherwise the resource is placed at the terrain ray intersection and can be picked up again with the same chord. Escape, blur, hidden-page and cancelled-pointer events return held resources safely. Camera dragging and Ctrl zoom are suppressed for this gesture in the drought scenario; native comparison controls stay unchanged. The existing Hand tools buttons remain available for touch users. Dropped bundles are session objects and are not part of the drought checkpoint.

Ctrl-grab acceptance: browser Ctrl-click on a tree created and released a wood bundle; repeating the gesture picked up and released that dropped bundle. The same gesture on a field released a food offering at the pointer. UI tests cover modifier-first release, pointer-first release, single delivery of the release event, focus loss, Escape and cancellation. Native and WASM builds passed.


### Whole-villager pickup

In the drought scene, hold Ctrl + left mouse on a villager to carry the existing villager. Release either to place them. Hand tools also offers Find villager, Pick up villager and Return held object. Escape, focus loss, leaving the scene and restarting return carried villagers to their pickup position.

Placement snaps within eight world units to clear, dry terrain using the same slope and obstacle clearance as worker navigation. A moved worker needs at least one reachable job. Invalid placement returns the villager safely. Carried workers pause progress and rewards, hide their work prop, and use the idle pose. Placed workers rebuild their routes and start a fresh trip without receiving a completion reward for the abandoned trip; cancellation preserves the interrupted trip.

Moved villagers and dropped resources are session-only. The current save schema still restores the original village positions on reload. Persisting physical village changes is the next milestone. Carry poses, throwing and impact damage are not implemented.
