# Shade web hosting

This is a Railway-compatible static file host, **not a playable game**.

## Local smoke test

```sh
python3 web/serve.py
curl http://localhost:8080/healthz
```

The health endpoint reports whether a compiled `openblack.html` entry point exists, **not** whether it works. Install authorized build artifacts into `web/dist/`.

## Railway

Create a service from GitHub repository `DanoADB/Shade`, branch `shade-development`. Set the service root to repository root and Dockerfile path to `web/Dockerfile` (if required by Railway). Railway supplies `PORT` automatically. Healthcheck path: `/healthz`. Deploying now provides a holding page only.

Never commit or publish proprietary Lionhead game data. An eventual public demo needs original or appropriately licensed replacement assets.

## Known blockers

- `src/main.cpp` defaults to Vulkan on non-Apple systems; there is no browser-compatible graphics backend in the `GraphicsBackend` enum.
- `src/CMakeLists.txt` uses obsolete `-s USE_SDL=2` Emscripten flags, while the engine requires SDL3 and uses SDL GPU.
- Host-side Slang shader compilation and SDL GPU WebGPU support need validation.
- Original assets are absent by design.

Build pipeline: `.github/workflows/shade-web.yml` is an initial compilation probe.
