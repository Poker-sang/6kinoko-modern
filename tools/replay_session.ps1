param(
    [ValidateSet('record','play')][string]$Mode = 'record',
    [string]$BaseDir = $PSScriptRoot,
    [string]$Session,
    [switch]$PrepareOnly
)
$ErrorActionPreference = 'Stop'
$base = (Resolve-Path -LiteralPath $BaseDir).Path
$sessions = Join-Path $base 'recordings'
if (-not (Test-Path -LiteralPath $sessions)) { New-Item -ItemType Directory -Path $sessions | Out-Null }
function Unique-Directory([string]$parent, [string]$prefix) {
    $path = Join-Path $parent ($prefix + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $path | Out-Null
    return $path
}
function File-Sha256([string]$path) {
    $hash = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead($path)
    try { return ([BitConverter]::ToString($hash.ComputeHash($stream))).Replace('-','').ToLowerInvariant() }
    finally { $stream.Dispose(); $hash.Dispose() }
}
function Inventory([string]$directory) {
    $entries = @(Get-ChildItem -LiteralPath $directory -File -Recurse | ForEach-Object {
        [ordered]@{ path=$_.FullName.Substring($directory.Length+1).Replace('\','/'); sha256=(File-Sha256 $_.FullName) }
    } | Sort-Object { $_.path })
    return ,$entries
}
function Identity($entries) {
    $text = ($entries | ForEach-Object { $_.path + "`n" + $_.sha256 + "`n" }) -join ''
    $hash = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes($text)))).Replace('-','').ToLowerInvariant() }
    finally { $hash.Dispose() }
}
function Runtime-Inventory {
    $files = @()
    foreach($name in @('kinoko_modern_gpu.exe','6kinoko_a.dat','6kinoko_b.dat','6kinoko_c.dat')) {
        $path=Join-Path $base $name
        $files += [ordered]@{path=$name; sha256=(File-Sha256 $path)}
    }
    foreach($name in @('fonts','shaders')) {
        if(-not (Test-Path -LiteralPath (Join-Path $base $name))) {throw "Missing runtime asset: $name"}
        foreach($entry in (Inventory (Join-Path $base $name))) {
            $files += [ordered]@{path=($name+'/'+$entry.path); sha256=$entry.sha256}
        }
    }
    return ,@($files | Sort-Object { $_.path })
}
# Hash in place; never copy the executable or immutable assets into a session.
$runtimeFiles=Runtime-Inventory
if ($Mode -eq 'record') {
    $sessionRoot = Unique-Directory $sessions 'session'
    $initial = Join-Path $sessionRoot 'initial'
    New-Item -ItemType Directory -Path $initial | Out-Null
    # Preserve writable DAT files, including saves/configs, but exclude archives.
    Get-ChildItem -LiteralPath $base -File -Filter '*.dat' | Where-Object {
        $_.Name -notin @('6kinoko_a.dat','6kinoko_b.dat','6kinoko_c.dat')
    } | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $initial }
    if (Test-Path -LiteralPath (Join-Path $base 'input-actions.cfg')) {
        Copy-Item -LiteralPath (Join-Path $base 'input-actions.cfg') -Destination $initial
    }
    $entries = Inventory $initial
    $identity = Identity (@($runtimeFiles)+@($entries))
    [ordered]@{version=2; identity=$identity; runtime=$runtimeFiles; files=$entries} | ConvertTo-Json -Depth 4 |
        Set-Content -LiteralPath (Join-Path $sessionRoot 'session-manifest.json') -Encoding UTF8
    $run = Join-Path $sessionRoot 'record'
    New-Item -ItemType Directory -Path $run | Out-Null
    [IO.File]::WriteAllText((Join-Path $sessions 'last-session.txt'),$sessionRoot,[Text.Encoding]::UTF8)
} else {
    if (-not $Session) { $Session = [IO.File]::ReadAllText((Join-Path $sessions 'last-session.txt')).Trim() }
    $sessionRoot = (Resolve-Path -LiteralPath $Session).Path
    $initial = Join-Path $sessionRoot 'initial'
    $manifest = Get-Content -LiteralPath (Join-Path $sessionRoot 'session-manifest.json') -Raw | ConvertFrom-Json
    if($manifest.version -ne 2) {throw 'Use the original retained game package to play a version-1 session.'}
    $entries = Inventory $initial
    $identity = Identity (@($runtimeFiles)+@($entries))
    if ($identity -cne $manifest.identity) { throw 'Executable, resources or initial saves/configuration changed; replay refused.' }
    if (-not (Test-Path -LiteralPath (Join-Path $sessionRoot 'record/session.krec'))) { throw 'No recording exists in this session.' }
    $run = Unique-Directory $sessionRoot 'play'
}
Get-ChildItem -LiteralPath $initial | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $run -Recurse }
if ((Identity (Inventory $run)) -cne (Identity $entries)) { throw 'Save/configuration copy hash verification failed.' }
$recording=Join-Path $sessionRoot 'record/session.krec'
$statusPath=Join-Path $run 'replay-status.txt'
$exe=Join-Path $base 'kinoko_modern_gpu.exe'
# Paths are absolute; SDL's entry point handles Unicode command-line arguments.
$arguments=@('--save-dir',$run,($(if($Mode -eq 'record'){'--record'}else{'--replay'})),$recording,'--replay-status',$statusPath,'--replay-identity',$identity)
[ordered]@{executable=$exe; arguments=$arguments} | ConvertTo-Json -Depth 3 |
    Set-Content -LiteralPath (Join-Path $run 'launch-arguments.json') -Encoding UTF8
Write-Output "Session: $sessionRoot"
Write-Output "Save directory: $run"
if ($PrepareOnly) { return }
$previousMode = $env:KINOKO_REPLAY_MODE
$previousTrace = $env:KINOKO_TRACE
try {
    $env:KINOKO_REPLAY_MODE = $null
    $env:KINOKO_TRACE = '0'
    # Every value here is a file/directory path or hex identity, never shell code.
    $quotedArguments=($arguments | ForEach-Object { '"'+$_+'"' }) -join ' '
    $process = Start-Process -FilePath $exe -ArgumentList $quotedArguments -WorkingDirectory $base -WindowStyle Normal -PassThru -Wait
    $result = @(Get-Content -LiteralPath $statusPath)
    $result | Write-Output
    if (-not ($result -match '^COMPLETED [0-9]+ frames$')) { throw "Session did not complete; see $statusPath" }
    if ($process.ExitCode -ne 0) { throw "Game exited with code $($process.ExitCode); retained session: $run" }
} finally {
    $env:KINOKO_REPLAY_MODE = $previousMode
    $env:KINOKO_TRACE = $previousTrace
}
