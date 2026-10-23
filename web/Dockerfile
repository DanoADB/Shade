# Package the verified full browser engine from web/dist/engine.
# The optional graphics probe is still built separately below.
FROM emscripten/emsdk:4.0.15 AS wasm-builder
WORKDIR /source
COPY web/engine-probe/ web/engine-probe/
RUN bash web/engine-probe/build.sh

FROM python:3.12-alpine
WORKDIR /app
COPY web/serve.py /app/web/serve.py
COPY web/dist/ /app/web/dist/
COPY --from=wasm-builder /source/web/dist/engine-probe/ /app/web/dist/engine-probe/
RUN test -s /app/web/dist/engine/openblack.html && test -s /app/web/dist/engine/openblack.js && test -s /app/web/dist/engine/openblack.wasm
RUN test -s /app/web/dist/engine/mobile-controls.js && test -s /app/web/dist/game-assets/manifest.json
RUN test -s /app/web/dist/engine/gameplay.js
ENV PYTHONUNBUFFERED=1
EXPOSE 8080
CMD ["python", "/app/web/serve.py"]
