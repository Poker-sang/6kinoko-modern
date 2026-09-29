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
    $name = $prefix + '-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
    $path = Join-Path $parent $name
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
if ($Mode -eq 'record') {
    foreach ($name in @('kinoko_modern_gpu.exe','6kinoko_a.dat','6kinoko_b.dat','6kinoko_c.dat','fonts','shaders')) {
        if (-not (Test-Path -LiteralPath (Join-Path $base $name))) { throw "Missing runtime asset: $name" }
    }
    $sessionRoot = Unique-Directory $sessions 'session'
    $initial = Join-Path $sessionRoot 'initial'
    New-Item -ItemType Directory -Path $initial | Out-Null
    foreach ($name in @('kinoko_modern_gpu.exe','fonts','shaders')) {
        Copy-Item -LiteralPath (Join-Path $base $name) -Destination $initial -Recurse
    }
    # Snapshot saves and legacy config as well as all three original DAT files.
    Get-ChildItem -LiteralPath $base -File -Filter '*.dat' | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $initial }
    if (Test-Path -LiteralPath (Join-Path $base 'input-actions.cfg')) {
        Copy-Item -LiteralPath (Join-Path $base 'input-actions.cfg') -Destination $initial
    }
    $entries = Inventory $initial
    $identity = Identity $entries
    [ordered]@{version=1; identity=$identity; files=$entries} | ConvertTo-Json -Depth 4 |
        Set-Content -LiteralPath (Join-Path $sessionRoot 'session-manifest.json') -Encoding UTF8
    $run = Join-Path $sessionRoot 'record'
    New-Item -ItemType Directory -Path $run | Out-Null
    [IO.File]::WriteAllText((Join-Path $sessions 'last-session.txt'),$sessionRoot,[Text.Encoding]::UTF8)
} else {
    if (-not $Session) { $Session = [IO.File]::ReadAllText((Join-Path $sessions 'last-session.txt')).Trim() }
    $sessionRoot = (Resolve-Path -LiteralPath $Session).Path
    $initial = Join-Path $sessionRoot 'initial'
    $manifest = Get-Content -LiteralPath (Join-Path $sessionRoot 'session-manifest.json') -Raw | ConvertFrom-Json
    $entries = Inventory $initial
    $identity = Identity $entries
    if ($manifest.version -ne 1 -or $identity -cne $manifest.identity) { throw 'Initial executable, resources, configuration or saves changed; replay refused.' }
    if (-not (Test-Path -LiteralPath (Join-Path $sessionRoot 'record/session.krec'))) { throw 'No recording exists in this session.' }
    $run = Unique-Directory $sessionRoot 'play'
}
Get-ChildItem -LiteralPath $initial | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $run -Recurse }
# Verify actual copied files, not only the initial directory.
if ((Identity (Inventory $run)) -cne $identity) { throw 'Session copy hash verification failed.' }
[IO.File]::WriteAllText((Join-Path $run 'session.identity'),$identity,[Text.Encoding]::ASCII)
[IO.File]::WriteAllText((Join-Path $run '.replay-session'),$Mode,[Text.Encoding]::ASCII)
if ($Mode -eq 'play') { Copy-Item -LiteralPath (Join-Path $sessionRoot 'record/session.krec') -Destination $run }
Write-Output "Session: $sessionRoot"
Write-Output "Run directory: $run"
if ($PrepareOnly) { return }
$previousMode = $env:KINOKO_REPLAY_MODE
$previousTrace = $env:KINOKO_TRACE
try {
    $env:KINOKO_REPLAY_MODE = $Mode
    $env:KINOKO_TRACE = '0'
    # This is the visible game requested by the launcher, not a background helper.
    $process = Start-Process -FilePath (Join-Path $run 'kinoko_modern_gpu.exe') -WorkingDirectory $run -WindowStyle Normal -PassThru -Wait
    $result = @(Get-Content -LiteralPath (Join-Path $run 'replay-status.txt'))
    $result | Write-Output
    if (-not ($result -match '^COMPLETED [0-9]+ frames$')) { throw "Session did not complete; see $run/replay-status.txt" }
    if ($process.ExitCode -ne 0) { throw "Game exited with code $($process.ExitCode); retained session: $run" }
} finally {
    $env:KINOKO_REPLAY_MODE = $previousMode
    $env:KINOKO_TRACE = $previousTrace
}
