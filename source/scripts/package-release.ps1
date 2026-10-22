[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Version,
    [Parameter(Mandatory=$true)][ValidateSet('windows-x86','windows-x64','windows-arm64','linux-x64','linux-arm64','macos-arm64')][string]$Platform,
    [Parameter(Mandatory=$true)][string]$StageDirectory,
    [string]$OutputDirectory
)
$ErrorActionPreference='Stop'
if($Version -notmatch '^[A-Za-z0-9._-]+$'){throw 'Version must be a safe filename component.'}
$stage=(Resolve-Path -LiteralPath $StageDirectory).Path
if(!$OutputDirectory){$OutputDirectory=Join-Path (Split-Path -Parent $PSScriptRoot) 'dist'}
$output=[IO.Path]::GetFullPath($OutputDirectory)
if($output -eq $stage -or $output.StartsWith($stage+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Archives must be outside staging.'}
$assets=$stage
$exe=Join-Path $stage 'blaze'
if($Platform.StartsWith('windows-')){$exe+='.exe'}
if($Platform -eq 'macos-arm64'){
    $assets=Join-Path $stage 'Blaze.app/Contents/Resources'
    $exe=Join-Path $stage 'Blaze.app/Contents/MacOS/Blaze'
}
foreach($name in @('dar','dat','midi','se','resources/rop.bin','resources/trig.bin','dat/Blz.ini','dat/BlzScr.ini','README.md','THIRD_PARTY_NOTICES.md')){
    if(!(Test-Path -LiteralPath (Join-Path $assets $name))){throw "Missing runtime content: $name"}
}
foreach($name in @('source','game-recomp','.git','dat/BlzCtrl.ini')){
    if(Test-Path -LiteralPath (Join-Path $assets $name)){throw "Private/source/user content in runtime package: $name"}
}
$bytes=[IO.File]::ReadAllBytes($exe)
$identity=[Text.Encoding]::ASCII.GetString($bytes)
if(!$identity.Contains('BLAZE_NATIVE_C:')){throw 'Native application identity missing.'}
if($identity.Contains('BLAZE_NATIVE_C: SDL3') -and !(Test-Path -LiteralPath (Join-Path $assets 'SDL3-LICENSE.txt'))){throw 'SDL3 candidate requires its license in the runtime package.'}
if($Platform.StartsWith('windows-')){
    if($bytes.Length -lt 64 -or $bytes[0] -ne 77 -or $bytes[1] -ne 90){throw 'Invalid PE.'}
    $pe=[BitConverter]::ToInt32($bytes,60)
    if($pe -lt 64 -or $pe -gt $bytes.Length-6 -or [BitConverter]::ToUInt32($bytes,$pe) -ne 0x4550){throw 'Invalid PE header.'}
    $expected=@{'windows-x86'=0x14c;'windows-x64'=0x8664;'windows-arm64'=0xaa64}[$Platform]
    if([BitConverter]::ToUInt16($bytes,$pe+4) -ne $expected){throw 'Wrong executable architecture.'}
}else{
    $description=(& file -b $exe) -join ' '
    if($LASTEXITCODE){throw 'Cannot inspect executable.'}
    $expected=@{'linux-x64'='ELF 64-bit.*x86-64';'linux-arm64'='ELF 64-bit.*(ARM aarch64|ARM64)';'macos-arm64'='Mach-O 64-bit.*arm64'}[$Platform]
    if($description -notmatch $expected){throw "Wrong executable architecture: $description"}
    if($Platform -eq 'macos-arm64'){
        & codesign --force --deep --sign - (Join-Path $stage 'Blaze.app')
        if($LASTEXITCODE){throw 'Ad-hoc signing failed.'}
        & codesign --verify --deep --strict (Join-Path $stage 'Blaze.app')
        if($LASTEXITCODE){throw 'App verification failed.'}
    }
}
New-Item -ItemType Directory -Path $output -Force | Out-Null
$base=Join-Path $output "blaze-$Version-$Platform"
if($Platform.StartsWith('windows-')){& tar -a -cf "$base.zip" -C $stage .}
elseif($Platform -eq 'macos-arm64'){& hdiutil create -ov -volname 'Blaze' -srcfolder $stage -format UDZO "$base.dmg"}
else{& tar -czf "$base.tar.gz" -C $stage .}
if($LASTEXITCODE){throw 'Packaging failed.'}
Write-Host "Packaged $base"
