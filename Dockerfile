# Build browser-compatible C++ rendering probe at deployment time.
# This is a graphics-port experiment, not the full openblack game.
FROM emscripten/emsdk:4.0.15 AS wasm-builder
WORKDIR /source
COPY web/engine-probe/ web/engine-probe/
RUN bash web/engine-probe/build.sh

FROM python:3.12-alpine
WORKDIR /app
COPY web/serve.py /app/web/serve.py
COPY web/dist/ /app/web/dist/
COPY --from=wasm-builder /source/web/dist/engine-probe/ /app/web/dist/engine-probe/
ENV PYTHONUNBUFFERED=1
EXPOSE 8080
CMD ["python", "/app/web/serve.py"]
