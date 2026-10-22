# Shade browser hosting

The static host serves the packaged full OpenBlack WebGL2 engine at `/engine/openblack.html`. The home page links to it, and existing `/model-preview/` bookmarks redirect there. The earlier JavaScript gameplay prototype remains at `/prototype/`; the small C++ graphics experiment remains at `/engine-probe/`.

## Prepare a release

Build and verify the full engine with `tools/shade/wasm.ps1 -Action Build`, then run `python tools/shade/package_engine.py`. Run `python tools/shade/package_game_assets.py` to package the prepared game files, then package the engine. The engine packager copies the HTML, JavaScript and WASM engine artifacts into `web/dist/engine/` and records their source revision, lengths and SHA-256 hashes in `manifest.json`. Include these packaged engine files in the reviewed deployment commit. Regenerate the package after engine or launcher changes; the current Docker images package this verified build rather than rebuilding the full engine toolchain.

Run `python web/serve.py` for a local smoke test. Open `/`, launch the engine, wait for the automatic game-file download, then select Village view or Village close-up. Hosted files are loaded into browser memory without requiring visitors to provide a folder. The optional folder picker is a fallback for development hosts without a game pack.

## Railway

`railway.json` selects the root `Dockerfile`. Both Dockerfiles copy `web/dist/engine/` and fail if the three engine artifacts are missing. Railway supplies PORT; health check is `/healthz`. That endpoint reports package presence and the engine manifest, not gameplay readiness. Engine files are served without caching so JavaScript and WASM from different builds are not mixed.

After the deployment commit is pushed, verify the live health manifest, `/model-preview/` redirect and `/engine/openblack.html`, then confirm the scene starts without selecting a folder. The deployment includes 718 prepared game files, compressed as content-addressed gzip objects (about 340 MB). Each decoded file is checked against its original length and SHA-256 before installation. Objects are cached by hash; the manifest is fetched fresh.

The Docker executable is unavailable on the current development machine, so the container build has not been tested here. The same Python static host and packaged files have been tested locally, including a successful browser folder import reproducing the original hand/village/tree scene. The existing GitHub `shade-web.yml` workflow is a separate source compilation experiment; it is not the artifact source for this package.

The engine currently renders original terrain materials and rest-pose models. Animated playback, water, complete interaction and a playable vertical slice remain in progress.


## Touch controls

The full-engine launcher has an on-screen movement pad, zoom, rotation and tilt controls. Touch controls appear automatically for coarse-pointer devices and can be toggled on desktop. The existing engine handles canvas dragging, pinch zoom and tapping to point the hand. Controls release on pointer cancellation, focus loss and hidden tabs; multiple fingers on the same action do not release it prematurely. Canvas/window sizes follow viewport and orientation changes, with a maximum rendering edge of 1280 pixels. Details are collapsed to keep the game visible, and supported browsers offer fullscreen.

Verification: WASM and native builds, `node web/tests/mobile-controls.test.cjs`, and browser checks at 390×844 and 844×390. Holding movement and zoom controls changed the engine camera without browser errors; both layouts fit without horizontal scrolling. Physical Android/iOS devices and native multitouch gestures have not yet been tested. The large current game pack and memory footprint remain a limitation for phones.
