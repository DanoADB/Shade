# Shade browser hosting

The static host serves the packaged full OpenBlack WebGL2 engine at `/engine/openblack.html`. The home page links to it, and existing `/model-preview/` bookmarks redirect there. The earlier JavaScript gameplay prototype remains at `/prototype/`; the small C++ graphics experiment remains at `/engine-probe/`.

## Prepare a release

Build and verify the full engine with `tools/shade/wasm.ps1 -Action Build`, then run `python tools/shade/package_engine.py`. This copies only the HTML, JavaScript and WASM engine artifacts into `web/dist/engine/` and records their source revision, lengths and SHA-256 hashes in `manifest.json`. Include these packaged engine files in the reviewed deployment commit. Regenerate the package after engine or launcher changes; the current Docker images package this verified build rather than rebuilding the full engine toolchain.

Run `python web/serve.py` for a local smoke test. Open `/`, launch the engine, choose a prepared game folder containing Data, Scripts and Audio, then select Village view or Village close-up. Folder selection reads the files into browser memory; it does not upload game data to the host. Original game files remain outside the deployment package.

## Railway

`railway.json` selects the root `Dockerfile`. Both Dockerfiles copy `web/dist/engine/` and fail if the three engine artifacts are missing. Railway supplies PORT; health check is `/healthz`. That endpoint reports package presence and the engine manifest, not gameplay readiness. Engine files are served without caching so JavaScript and WASM from different builds are not mixed.

After the deployment commit is approved and pushed, verify the live health manifest, `/model-preview/` redirect and `/engine/openblack.html`, then load local game files. Railway cannot read the development machine's desktop folder automatically.

The Docker executable is unavailable on the current development machine, so the container build has not been tested here. The same Python static host and packaged files have been tested locally, including a successful browser folder import reproducing the original hand/village/tree scene. The existing GitHub `shade-web.yml` workflow is a separate source compilation experiment; it is not the artifact source for this package.

The engine currently renders original terrain materials and rest-pose models. Animated playback, water, complete interaction and a playable vertical slice remain in progress.
