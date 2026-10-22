param([string]$InputModel = (Join-Path $PSScriptRoot '../../.local/game-data-base/CreatureMesh/C_Cow_Boned.l3d'))
$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '../..')
try {
 & ./.local/emsdk/upstream/emscripten/em++.bat web/model-preview/export.cpp components/l3d/src/L3DFile.cpp -Icomponents/l3d/include -Iapps/l3dtool -std=c++23 -O2 -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1 -o .local/model-export.js
 if($LASTEXITCODE -ne 0){throw 'Exporter compilation failed'}
 & ./.local/emsdk/node/24.19.0_64bit/node.exe .local/model-export.js $InputModel .local/cow.json
 if($LASTEXITCODE -ne 0){throw 'Model export failed'}
} finally {Pop-Location}
