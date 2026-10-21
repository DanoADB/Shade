#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p ../dist/engine-probe
em++ main.cpp -std=c++17 -O2 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH=1 -sASSERTIONS=1 -o ../dist/engine-probe/probe.js
cp index.html ../dist/engine-probe/index.html
printf 'Built web/dist/engine-probe/\n'
