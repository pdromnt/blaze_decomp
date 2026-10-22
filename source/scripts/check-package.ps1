param([Parameter(Mandatory=$true)][string]$Platform, [string]$ArchiveDirectory='source/dist')
$ErrorActionPreference='Stop'
$archive=Get-ChildItem -LiteralPath $ArchiveDirectory -File | Where-Object { $_.Name -notlike '*.sha256' }
if(@($archive).Count -ne 1){throw 'Expected one runtime archive.'}
$expected=(Get-Content "$($archive.FullName).sha256").Split(' ')[0]
if((Get-FileHash $archive.FullName).Hash -ne $expected){throw 'Archive checksum mismatch.'}
$destination=Join-Path $env:RUNNER_TEMP 'Blaze package test'
New-Item -ItemType Directory -Force $destination | Out-Null
$mounted=$false
try {
    if($Platform -eq 'macos-arm64'){
        & hdiutil attach -readonly -nobrowse -mountpoint $destination $archive.FullName
        if($LASTEXITCODE){throw 'Cannot mount DMG.'}
        $mounted=$true
        $binary=Join-Path $destination 'Blaze.app/Contents/MacOS/Blaze'
    } elseif($Platform.StartsWith('windows-')){
        Expand-Archive -LiteralPath $archive.FullName -DestinationPath $destination
        $binary=Join-Path $destination 'blaze.exe'
    } else {
        & tar -xzf $archive.FullName -C $destination
        if($LASTEXITCODE){throw 'Cannot extract runtime.'}
        $binary=Join-Path $destination 'blaze'
    }
    $start=@{FilePath=$binary; ArgumentList=@('--hidden','--audio','--frames','50','--seed','1','--settings','none'); WorkingDirectory=$env:RUNNER_TEMP; PassThru=$true}
    if($IsWindows){$start.WindowStyle='Hidden'}
    $process=Start-Process @start
    try {
        if(!$process.WaitForExit(15000)){throw 'Packaged application timed out.'}
        if($process.ExitCode -ne 0){throw "Packaged application exited $($process.ExitCode)."}
    } finally {
        if(!$process.HasExited){$process.Kill(); $process.WaitForExit()}
        $process.Dispose()
    }
} finally {
    if($mounted){& hdiutil detach $destination; if($LASTEXITCODE){throw 'Cannot unmount test DMG.'}}
}
