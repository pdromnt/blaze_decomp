[CmdletBinding()]
param(
    [ValidateSet('x86','x64','ARM64')][string]$Architecture='x64',
    [ValidateSet('Debug','RelWithDebInfo','Release')][string]$Configuration='RelWithDebInfo',
    [ValidateSet('WIN32','SDL3')][string]$Backend='WIN32',
    [switch]$Tests,
    [switch]$NoDeploy
)
$ErrorActionPreference='Stop'
$source=Split-Path -Parent $PSScriptRoot
$root=Split-Path -Parent $source
$cmake=Get-Command cmake.exe -ErrorAction SilentlyContinue
if($cmake){$cmake=$cmake.Source}else{
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if(Test-Path -LiteralPath $vswhere){$cmake=& $vswhere -latest -products '*' -find 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' | Select-Object -First 1}
}
if(!$cmake){throw 'CMake 3.24+ and Visual Studio C/C++ tools are required.'}
$suffix=if($Backend -eq 'SDL3'){'-sdl3'}else{''}
$build=Join-Path $source ('build/windows-'+$Architecture.ToLowerInvariant()+$suffix)
$platform=if($Architecture -eq 'x86'){'Win32'}else{$Architecture}
$testOption=if($Tests){'ON'}else{'OFF'}
& $cmake -S $source -B $build -A $platform "-DBUILD_TESTING=$testOption" -DBLAZE_BUILD_APPLICATION=ON "-DBLAZE_PLATFORM=$Backend"
if($LASTEXITCODE){throw 'Configuration failed.'}
& $cmake --build $build --config $Configuration --parallel 4
if($LASTEXITCODE){throw 'Build failed.'}
if($Tests){
    & (Join-Path (Split-Path -Parent $cmake) 'ctest.exe') --test-dir $build -C $Configuration --output-on-failure
    if($LASTEXITCODE){throw 'Native checks failed; root executable was not replaced.'}
}
if(!$NoDeploy){
    $destination=Join-Path $root 'blaze.exe'
    $originalHash='02332EA9C4746CE6255F044097C4B095F44C539F265927A3CA76CA01737FA74D'
    if(Test-Path -LiteralPath $destination){
        if((Get-FileHash -LiteralPath $destination).Hash -eq $originalHash){
            $preserved=Join-Path $root 'game-recomp/original/preserved/blaze.exe'
            if(!(Test-Path -LiteralPath $preserved) -or (Get-FileHash -LiteralPath $preserved).Hash -ne $originalHash){
                throw 'Canonical original must be preserved before replacing root blaze.exe.'
            }
        }elseif(![Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($destination)).Contains('BLAZE_NATIVE_C:')){
            throw 'Refusing to replace an unknown root executable. Preserve it first.'
        }
    }
    New-Item -ItemType Directory -Path (Join-Path $root 'resources') -Force | Out-Null
    foreach($name in @('rop.bin','trig.bin')) {Copy-Item -LiteralPath (Join-Path $source "resources/$name") -Destination (Join-Path $root "resources/$name") -Force}
    foreach($name in @('Blz.ini','BlzScr.ini')) {
        $settings=Join-Path $root "dat/$name"
        if(!(Test-Path -LiteralPath $settings)){Copy-Item -LiteralPath (Join-Path $source "resources/defaults/$name") -Destination $settings}
    }
    if($Backend -eq 'SDL3') {
        Copy-Item -LiteralPath (Join-Path $build 'bin/libfluidsynth-3.dll') -Destination $root -Force
        Copy-Item -LiteralPath (Join-Path $build 'bin/resources/GeneralUser-GS.sf2') -Destination (Join-Path $root 'resources/GeneralUser-GS.sf2') -Force
        foreach($name in @('SDL3','FluidSynth','GeneralUser-GS','GCEM')) {
            Copy-Item -LiteralPath (Join-Path $source "third_party/$name-LICENSE.txt") -Destination $root -Force
        }
    }
    Copy-Item -LiteralPath (Join-Path $build 'bin/blaze.exe') -Destination $destination -Force
    Write-Host "Built $destination; local INIs preserved."
}else{Write-Host "Built $build/bin/blaze.exe; deployment skipped."}
