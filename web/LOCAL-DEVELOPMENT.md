# Local Shade development (Windows)

Run commands from this Shade checkout. Branch: shade-development, initial commit 7e966cd5.
Docker was not installed on this machine; Python 3.14 and a checkout-local Emscripten 4.0.15 SDK were used successfully.

## JavaScript gameplay prototype

    python web/serve.py

Open http://localhost:8080/. Edit web/dist/prototype.js and web/dist/index.html, then refresh the browser (Ctrl+F5 if cached). No compilation is needed. Set $env:PORT = '8081' before starting to change the port. Stop a foreground server with Ctrl+C.

The server started during setup is running in the background. Its PID is in .local/server.pid, with output in .local/server.out.log and .local/server.err.log. Stop that instance with:

    Stop-Process -Id ([int](Get-Content .local/server.pid))

The existing server binds 0.0.0.0, so Windows firewall settings determine LAN access.

## C++ WebGL2 probe

The SDK is installed under .local/emsdk and activated without permanent system environment changes. Rebuild after editing web/engine-probe/main.cpp or index.html:

    & ./web/engine-probe/build.ps1

Open http://localhost:8080/engine-probe/. The script uses the same C++17, optimization, WebGL2, memory-growth and assertion options as build.sh, and copies the HTML shell. It generates probe.js and probe.wasm in web/dist/engine-probe/. Refresh after rebuilding. Arrows/WASD rotate and tilt; wheel zooms.

To install the same SDK in a fresh checkout:

    git clone https://github.com/emscripten-core/emsdk.git .local/emsdk
    ./.local/emsdk/emsdk.bat install 4.0.15
    ./.local/emsdk/emsdk.bat activate 4.0.15
    & ./web/engine-probe/build.ps1

On a Bash-based setup with em++ available, the original workflow remains:

    bash web/engine-probe/build.sh
    python web/serve.py

## Docker alternative (not tested here)

With Docker installed and its engine running, from repository root:

    docker build -f web/Dockerfile -t shade-web:dev .
    docker run --rm --name shade-web -p 127.0.0.1:8080:8080 shade-web:dev

Stop the Python server first or choose another host port. Docker builds the probe with emscripten/emsdk:4.0.15 and serves both demos using Python 3.12 Alpine. Rebuild the image after edits.

## Checks and limits

GET /, /prototype.js, /engine-probe/, /engine-probe/probe.js, /engine-probe/probe.wasm and /healthz should return 200. WASM should use application/wasm. The server supplies COOP same-origin and COEP require-corp headers.

/healthz reports engine_artifact_present:false because openblack.html is absent. This is expected: it does not check the prototype or probe. Neither demo is the full openblack game. The first probe request returned 404 before compilation; compilation creates the route.

SDK files, generated probe artifacts and server logs are excluded locally via .git/info/exclude. No Railway service is required.

## Prototype save slot
Use Save world before closing and Load world to restore it. Loading pauses the world and preserves learned actions, trust, resources and movement targets. Saves use localStorage for this browser and origin; another browser or port has a separate save. New world leaves the saved slot intact. No offline simulation is applied.

## Original cow preview

The local preview uses openblack's C++ L3D reader, compiled separately from the engine. It exports the bone-transformed rest pose and the embedded RGBA4 textures. There is no animation, morph blending or gameplay integration yet. It is a preview of the specific supplied cow, not a complete L3D renderer.

Rebuild/export from repository root:

    & ./web/model-preview/export.ps1

Start a foreground preview server (stop the current background server first):

    $env:SHADE_COW_PREVIEW = (Resolve-Path .local/cow.json).Path
    python web/serve.py

Open http://localhost:8080/model-preview/. Drag to orbit, wheel to zoom, Reset camera to restore the initial view.

When SHADE_COW_PREVIEW is set, the server binds to 127.0.0.1 and serves that exact exported file at /local-cow.json. The original data and export remain in locally ignored .local, outside web/dist. Docker does not include or enable these assets. Unset SHADE_COW_PREVIEW for the ordinary static-host mode.

The model-preview route now contains the first combined 3D village scene. renderer.js provides WebGL rendering and camera controls; scene.js builds the placeholder world and positions the optional cow; viewer.js loads assets and reports status. Without the local export, a placeholder creature is displayed.

## Original Celtic village assets

The scene now uses AllMeshes.g3d IDs 74–76 (BuildingCeltic1–3), 443 (PersonCelticFemaleA1), and 446 (PersonCelticMaleA1), as named in src/3D/AllMeshes.h. Four houses and four villagers are placed with normalized scene sizes; these are not validated original-game scale ratios. Collision/helper geometry and nonzero construction-status submeshes are omitted. The exporter does not yet implement all L3D materials, animation or LOD selection.

Reproduce the cow plus village export from repository root after extracting the base game:

    python -m pip install --target .local/python-libs Pillow
    & ./web/model-preview/import-village.ps1

This uses openblack's PackFile and L3DFile readers and Pillow to decode the separate DDS textures. Original data, intermediate DDS/L3D files and final JSON stay in .local. The existing loopback-only /local-cow.json endpoint serves the combined scene data. Ground, paths and trees remain original placeholders. The 2D gameplay page still uses its prototype drawings.

## Original trees and grass

import-village.ps1 now also imports TreeBeech (571), TreeOak (583) and TreePine (590) from AllMeshes.g3d. Original DDS textures are decoded with Pillow. Six trees are placed using scene-specific heights.

The grass surface uses material index 1 (terrain type 18) from the supplied Landscape/Land1.lnd. extract-terrain.cpp uses openblack's LNDFile reader and decodes its B5G5R5A1 texels into RGB. This tile was visually inspected before selection. The scene repeats it 12 by 10 times across the flat island; this is not the original landscape geometry or its material blending.

Rerun import-village.ps1 to reproduce all assets, including terrain texture export. The terrain utility needs a 4 MiB WASM stack for the LND reader. Generated PPM and JSON files remain under .local. Without the local assets, the original placeholder trees and ground remain available.

## Shared 2D / 3D gameplay

simulation.js now owns simulation, UI controls and local save/load for both pages; prototype.js draws the 2D view and the model-preview modules draw the 3D view. The 3D renderer reads state snapshots and translates/turns the cow from its movement target. Walking animation and rain graphics are still pending. Wait advances 12 simulated seconds even while paused. New world resets the live world without clearing the save slot. Both pages share one localStorage save slot on the same browser origin; live worlds in separate tabs are independent.

Movement is tuned to 12 simulation units per second (three simulated seconds per real second), so crossing to the village takes roughly eight real seconds. This affects the original 2D prototype as well. Only arrival at the feeding target grants a food/help reward.

Run the simulation checks from repository root:

    & ./.local/emsdk/node/24.19.0_64bit/node.exe web/tests/simulation.test.cjs
