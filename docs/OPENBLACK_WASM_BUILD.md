# Full-engine WebAssembly build result — October 22, 2026

## Verified outcome

The full openblack target configured, compiled and linked with Emscripten 4.0.15 on Windows. This is the engine target, not the JavaScript prototype or procedural probe.

Release outputs in .local/wasm-build/bin:
- openblack.html: 19,601 bytes
- openblack.js: 239,783 bytes
- openblack.wasm: 3,501,990 bytes

Executing openblack.js --help under the SDK's Node runtime returned exit code 0 and printed the engine's options. A real browser loaded the generated artifacts, ran the main entry point and initialized SDL3 3.4.18 and EnTT 3.16.0.

## Runtime gate — not yet passed

Browser startup with `-g /game -l stdout` stops at:

    Failed to create SDL3 GPU device: SDL_HINT_GPU_DRIVER vulkan unsupported!
    Failed to create renderer
    Failed to initialize engine services.

No original assets were mounted for this renderer-first smoke test. Rendering service creation fails before the game-data check. A working renderer and a user-selected filesystem mount are both still required. Do not report the linked engine as playable in a browser.

The generated --emrun shell takes command-line arguments from the URL query, overwriting Module.arguments. The tested local URL was http://localhost:8082/openblack.html?-g&/game&-l&stdout. This is a diagnostic page, not a playable demo.

## Fixes made

1. Corrected the Emscripten preset's toolchain path relative to the SDK root.
2. Added Windows to the native SDL3 Vulkan feature selection; native Land1 rendering was verified separately.
3. Added repeatable native and WASM helper scripts. Cross builds import the already-built native shader compiler and include its runtime-library directory in their temporary PATH.
4. Disabled C++ module scanning for the engine's WASM build; the engine does not declare module units.
5. An initial OpenAL Soft WASM build failed in emscan-deps. A local scan-disabled port override allowed it to build, but final engine linking then failed on missing tss_get. This proved that getting the dependency to compile was insufficient.
6. Browser builds now use Emscripten's Web Audio-backed OpenAL shim instead of native OpenAL Soft. Native builds retain OpenAL Soft. Optional OpenAL Soft logging callbacks are excluded on Emscripten.
7. Set a 4 MiB WASM stack for engine/parser workloads.

After these changes, the final engine link passed. The native target also rebuilt successfully after the conditional audio changes. Audible browser output has not been tested because startup stops before audio initialization.

## Reproduce

From repository root, after preparing/building the native host tools and installing the local SDK:

    & ./tools/shade/wasm.ps1 -Action Build
    & ./.local/emsdk/node/24.19.0_64bit/node.exe .local/wasm-build/bin/openblack.js --help

For a local diagnostic host:

    python -m http.server 8082 --bind 127.0.0.1 --directory .local/wasm-build/bin

Logs: .local/wasm-configure.log (first dependency failure), .local/wasm-build.log (OpenAL Soft linker failure), .local/wasm-webaudio-build.log (successful final build), .local/wasm-help.log.

## Next implementation

Implement a browser rendering backend behind the engine's rendering/resource interfaces. Validate one engine-controlled terrain pass before adding sky, meshes/skinning, vegetation, water/reflections and debug UI. Retain the native renderer as the visual reference. A successful procedural or JS rendering demo is not acceptance for an engine pass. Then add a local browser asset mount and test the same original island against the native baseline.
