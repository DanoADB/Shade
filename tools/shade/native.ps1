param([ValidateSet('Configure','Build','Run')][string]$Action='Build')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$install){throw 'Visual Studio C++ Build Tools were not found.'}
$cmake=Join-Path $install 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ninja=Join-Path $install 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja'
$vcvars=Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
$local=Join-Path $repo '.local'
$build=Join-Path $local 'native-build'
New-Item -ItemType Directory -Force $local | Out-Null
if($Action -eq 'Run'){
 $env:PATH=(Join-Path $build 'vcpkg_installed/x64-windows/bin')+';'+$env:PATH
 $exe=Join-Path $build 'bin/openblack.exe'
 if(!(Test-Path $exe)){throw 'Build openblack before running it.'}
 $data=Join-Path $local 'native-game-data'
 if(!(Test-Path (Join-Path $data 'Data/AllMeshes.g3d'))){throw 'Run prepare_native_data.py before starting the engine.'}
 & $exe -g $data --start-level Land1.txt
 exit $LASTEXITCODE
}
Push-Location $repo
try {
 if(!(Test-Path 'vcpkg/vcpkg.exe')){
  & git submodule update --init --depth 1 vcpkg
  if($LASTEXITCODE -ne 0){throw 'vcpkg checkout failed'}
  & ./vcpkg/bootstrap-vcpkg.bat -disableMetrics
  if($LASTEXITCODE -ne 0){throw 'vcpkg bootstrap failed'}
 }
 $commands=@('@echo off',('call "'+$vcvars+'"'),'if errorlevel 1 exit /b %errorlevel%',('set "PATH='+$ninja+';'+(Join-Path $build 'vcpkg_installed/x64-windows/bin')+';%PATH%"'),('"'+$cmake+'" -S . -B .local/native-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPENBLACK_USE_VCPKG=ON -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_HOST_TRIPLET=x64-windows -DOPENBLACK_BUILD_SHADER_COMPILER=ON'),'if errorlevel 1 exit /b %errorlevel%')
 if($Action -eq 'Build'){$commands+=('"'+$cmake+'" --build .local/native-build --target openblack --parallel 4')}
 $batch=Join-Path $local 'native-workflow.cmd'
 $commands | Set-Content $batch
 & $batch
 if($LASTEXITCODE -ne 0){throw "Native $Action failed ($LASTEXITCODE)."}
}finally{Pop-Location}
