param([Parameter(Mandatory=$true)][string]$OutputRoot,[Parameter(Mandatory=$true)][string]$SessionScript)
$ErrorActionPreference='Stop'
$scriptPath=(Resolve-Path -LiteralPath $SessionScript).Path
$fixture=Join-Path $OutputRoot ('launcher-fixture-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture | Out-Null
foreach($name in @('kinoko_modern_gpu.exe','6kinoko_a.dat','6kinoko_b.dat','6kinoko_c.dat','marisaA.dat','keyconfig.dat','input-actions.cfg')) {
    [IO.File]::WriteAllText((Join-Path $fixture $name),('fixture only, never execute: '+$name))
}
foreach($name in @('fonts','shaders')) {
    New-Item -ItemType Directory -Path (Join-Path $fixture $name) | Out-Null
    [IO.File]::WriteAllText((Join-Path $fixture "$name/fixture"),'test asset')
}
& $scriptPath -Mode record -BaseDir $fixture -PrepareOnly
$session=[IO.File]::ReadAllText((Join-Path $fixture 'recordings/last-session.txt')).Trim()
$original=[IO.File]::ReadAllText((Join-Path $fixture 'marisaA.dat'))
[IO.File]::WriteAllText((Join-Path $session 'record/marisaA.dat'),'changed during recording')
[IO.File]::WriteAllText((Join-Path $fixture 'marisaA.dat'),'changed outside session')
[IO.File]::WriteAllText((Join-Path $session 'record/session.krec'),'launcher test only; runtime validates real replay bytes separately')
& $scriptPath -Mode play -BaseDir $fixture -Session $session -PrepareOnly
$play=Get-ChildItem -LiteralPath $session -Directory -Filter 'play-*' | Select-Object -First 1
if([IO.File]::ReadAllText((Join-Path $play.FullName 'marisaA.dat')) -cne $original) {throw 'Playback did not restore initial save'}
if([IO.File]::ReadAllText((Join-Path $fixture 'marisaA.dat')) -cne 'changed outside session') {throw 'Launcher modified normal save'}
[IO.File]::AppendAllText((Join-Path $session 'initial/marisaA.dat'),'tamper')
$rejected=$false
try { & $scriptPath -Mode play -BaseDir $fixture -Session $session -PrepareOnly } catch { $rejected=$true }
if(-not $rejected) {throw 'Changed snapshot was accepted'}
Write-Output "Launcher isolation, copied-file hashes and snapshot rejection passed; retained: $fixture"
