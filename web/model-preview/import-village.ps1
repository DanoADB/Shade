$ErrorActionPreference='Stop'
Push-Location (Join-Path $PSScriptRoot '../..')
try {
 & ./web/model-preview/export.ps1
 if($LASTEXITCODE -ne 0){throw 'Cow export failed'}
 & ./.local/emsdk/upstream/emscripten/em++.bat web/model-preview/extract-pack.cpp components/pack/src/PackFile.cpp -Icomponents/pack/include -std=c++23 -O2 -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1 -o .local/pack-extract.js
 if($LASTEXITCODE -ne 0){throw 'Pack extractor build failed'}
 & ./.local/emsdk/node/24.19.0_64bit/node.exe .local/pack-extract.js .local/game-data-base/Other_Graphics/AllMeshes.g3d .local/village-models
 if($LASTEXITCODE -ne 0){throw 'Pack extraction failed'}
 foreach($id in @(74,75,76,443,446,571,583,590)){
  & ./.local/emsdk/node/24.19.0_64bit/node.exe .local/model-export.js ".local/village-models/$id.l3d" ".local/village-models/$id.json"
  if($LASTEXITCODE -ne 0){throw "Model $id export failed"}
 }
 & ./.local/emsdk/upstream/emscripten/em++.bat web/model-preview/extract-terrain.cpp components/lnd/src/LNDFile.cpp -Icomponents/lnd/include -std=c++23 -O2 -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=4194304 -o .local/terrain-export.js
 if($LASTEXITCODE -ne 0){throw 'Terrain reader build failed'}
 & ./.local/emsdk/node/24.19.0_64bit/node.exe .local/terrain-export.js .local/game-data-base/Landscape/Land1.lnd .local/terrain-textures
 if($LASTEXITCODE -ne 0){throw 'Terrain texture export failed'}
 python web/model-preview/assemble-village.py
 if($LASTEXITCODE -ne 0){throw 'Village assembly failed'}
} finally {Pop-Location}
