param([string]$SdkPath = (Join-Path $PSScriptRoot '../../.local/emsdk'))
$ErrorActionPreference = 'Stop'
$compiler = Join-Path $SdkPath 'upstream/emscripten/em++.bat'
if (!(Test-Path $compiler)) { throw "Emscripten missing at $SdkPath. Install and activate SDK 4.0.15 first." }
$output = Join-Path $PSScriptRoot '../dist/engine-probe'
New-Item -ItemType Directory -Force -Path $output | Out-Null
& $compiler (Join-Path $PSScriptRoot 'main.cpp') -std=c++17 -O2 '-sMIN_WEBGL_VERSION=2' '-sMAX_WEBGL_VERSION=2' '-sALLOW_MEMORY_GROWTH=1' '-sASSERTIONS=1' '-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_shade_load_mesh' -o (Join-Path $output 'probe.js')
if ($LASTEXITCODE -ne 0) { throw "Probe build failed with exit code $LASTEXITCODE" }
Copy-Item (Join-Path $PSScriptRoot 'index.html') (Join-Path $output 'index.html')
Write-Host "Built $output"
