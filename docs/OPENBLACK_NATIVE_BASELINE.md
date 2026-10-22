# Native openblack baseline — October 22, 2026

## Verified

The engine was built from this Shade checkout with Visual Studio Build Tools 2026 (MSVC 14.51), CMake 4.2.3-msvc3, Ninja and pinned vcpkg submodule e456309491fd875487bf02b8a0dca76c1b1b00cc. The full openblack executable linked successfully.

Land1 loaded with data assembled from the user's disk copies at C:\Users\danod\OneDrive\Desktop\B&W. Native window inspection confirmed original terrain, textures, sky, ocean/reflections, vegetation and the hand cursor. A left-button drag changed the camera view. Screenshot evidence: .local/native-baseline.png.

The engine is running natively, not inside the browser. Audio output, creature animation/behavior, complete village interactions and saving have not been verified in this baseline.

## Fix required

First launch failed with `SDL_HINT_GPU_DRIVER vulkan unsupported!`. The root manifest requested SDL3's vulkan feature only for Linux/Android, despite the renderer explicitly choosing Vulkan on Windows. Added Windows to that feature selection, rebuilt SDL3 and relinked. The second launch rendered successfully using the machine's NVIDIA RTX 4080.

Shader generation initially logged missing optional `slang-glslang`/`spirv-opt` loader diagnostics while still producing headers and a linked application. The native/wasm helper scripts include the installed host-library directory in their temporary PATH. With that PATH correction, all eight shader sources were regenerated into a separate validation directory with exit code 0 and an empty diagnostic log. The first pass was not a clean shader-tool run; the corrected host-tool validation passed.

An optional clang-format download failed; that formatting target was disabled by CMake and did not prevent the engine build. minizip emitted a deprecation warning.

## Existing engine gaps observed

The successful Land1 startup logged unsupported script commands, including SetLandNumber, SetTownUninhabitable, CreatePlannedAbode and CreateMist. These are existing source implementations that log "not implemented", not browser-port regressions. Do not describe this baseline as complete original-game parity.

## Reproduce from repository root

    python tools/shade/prepare_native_data.py
    & ./tools/shade/native.ps1 -Action Build
    & ./tools/shade/native.ps1 -Action Run

prepare_native_data.py expects .local/game-data-base from the prior cabinet extraction and creates .local/native-game-data. It does not run the original installer, apply game patches or modify the disk copies.

Logs: .local/native-configure.log, .local/native-build.log, .local/native-vulkan-build.log, .local/openblack.log. The game log contains the failed first launch followed by the successful second launch; evaluate errors in the appropriate run.

## Next browser gate

Run tools/shade/wasm.ps1 to attempt configuration of the full engine with Emscripten 4.0.15 and wasm32-emscripten dependencies, importing the already-built native shader compiler. This path now produces openblack.html, openblack.js and openblack.wasm. Browser execution reaches SDL initialization but cannot create the hardcoded Vulkan renderer. It is not yet a rendered browser game. RendererSdl3Gpu currently hardcodes Vulkan and SPIR-V and will require a browser-compatible implementation.
