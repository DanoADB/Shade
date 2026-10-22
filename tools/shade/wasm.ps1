param([ValidateSet('Configure','Build')][string]$Action='Configure')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$local=Join-Path $repo '.local'
$hostCompiler=Join-Path $local 'native-build/bin/slang-sdl3-gpu-compiler.exe'
if(!(Test-Path $hostCompiler)){throw 'Build the native host tools with native.ps1 first.'}
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$install){throw 'Visual Studio C++ Build Tools were not found.'}
$cmake=Join-Path $install 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ninja=Join-Path $install 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja'
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$emsdk=Join-Path $local 'emsdk'
if(!(Test-Path (Join-Path $emsdk 'upstream/emscripten/emcc.bat'))){throw 'Install/activate the local Emscripten 4.0.15 SDK first.'}
$hostDirectory=Join-Path $local 'host-tools'
New-Item -ItemType Directory -Force $hostDirectory | Out-Null
$hostPath=$hostCompiler.Replace('\','/')
@"
add_executable(slang-sdl3-gpu-compiler IMPORTED GLOBAL)
set_target_properties(slang-sdl3-gpu-compiler PROPERTIES IMPORTED_LOCATION "$hostPath")
"@ | Set-Content (Join-Path $hostDirectory 'slang-sdl3-gpu-compiler-config.cmake')
$commands=@('@echo off',('call "'+$vcvars+'"'),'if errorlevel 1 exit /b %errorlevel%',('set "PATH='+$ninja+';'+(Join-Path $local 'native-build/vcpkg_installed/x64-windows/bin')+';%PATH%"'),('set "EMSDK='+$emsdk+'"'),'set "EMSCRIPTEN_ROOT=%EMSDK%/upstream/emscripten"',('"'+$cmake+'" -S . -B .local/wasm-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_SCAN_FOR_MODULES=OFF -DOPENBLACK_USE_VCPKG=ON -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DVCPKG_HOST_TRIPLET=x64-windows -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="%EMSDK%/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" -DOPENBLACK_BUILD_SHADER_COMPILER=OFF -Dslang-sdl3-gpu-compiler_DIR="'+$hostDirectory+'"'),'if errorlevel 1 exit /b %errorlevel%')
if($Action -eq 'Build'){$commands+=('"'+$cmake+'" --build .local/wasm-build --target openblack --parallel 4')}
$batch=Join-Path $local 'wasm-workflow.cmd'
$commands | Set-Content $batch
Push-Location $repo
try{& $batch;if($LASTEXITCODE -ne 0){throw "WASM $Action failed ($LASTEXITCODE)."}}finally{Pop-Location}
