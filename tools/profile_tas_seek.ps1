param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$Replay,
    [Parameter(Mandatory=$true)][string]$InitialDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [int]$Iterations=3
)
$ErrorActionPreference='Stop'
$Executable=(Resolve-Path -LiteralPath $Executable).Path
$Replay=(Resolve-Path -LiteralPath $Replay).Path
$InitialDirectory=(Resolve-Path -LiteralPath $InitialDirectory).Path
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $OutputDirectory){throw 'Use a fresh output directory'}
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
$bytes=[IO.File]::ReadAllBytes($Replay)
if([Text.Encoding]::ASCII.GetString($bytes,0,8) -ne 'KINORPL1'){throw 'Use an unpacked engine replay'}
$identity=[Text.Encoding]::ASCII.GetString($bytes,16,64)
$frames=[BitConverter]::ToUInt64($bytes,$bytes.Length-16)
if($frames -lt 2){throw 'At least two frames required'}
function Read-State([string]$directory){
    try{[IO.File]::ReadAllText((Join-Path $directory 'state.txt')).Trim().Split(' ')}catch{$null}
}
function Wait-State($process,[string]$directory,[long]$sequence,[long]$completed){
    $watch=[Diagnostics.Stopwatch]::StartNew()
    while($watch.Elapsed.TotalSeconds -lt 180){
        $state=Read-State $directory
        if($state -and $state.Length -eq 5 -and $state[1] -eq $sequence -and $state[2] -eq $completed -and $state[4] -eq 'paused'){return}
        if($process.HasExited){throw 'Replay engine exited unexpectedly'}
        if(Test-Path -LiteralPath (Join-Path $directory 'error.txt')){throw [IO.File]::ReadAllText((Join-Path $directory 'error.txt'))}
        Start-Sleep -Milliseconds 5
    }
    throw 'Replay seek timed out'
}
function Command([string]$directory,[string]$text){
    $temporary=Join-Path $directory 'command.tmp'
    [IO.File]::WriteAllText($temporary,$text,[Text.UTF8Encoding]::new($false))
    Move-Item -LiteralPath $temporary -Destination (Join-Path $directory 'command.txt') -Force
}
$results=@()
for($iteration=1;$iteration -le $Iterations;$iteration++){
    $run=Join-Path $OutputDirectory "run-$iteration"
    $bridge=Join-Path $run 'bridge';$saves=Join-Path $run 'saves'
    New-Item -ItemType Directory -Path $bridge,$saves | Out-Null
    Get-ChildItem -LiteralPath $InitialDirectory -File | Copy-Item -Destination $saves
    $output=Join-Path $run 'output.krec';$status=Join-Path $run 'replay-status.txt'
    $start=[Diagnostics.ProcessStartInfo]::new($Executable)
    $start.UseShellExecute=$false;$start.WorkingDirectory=Split-Path $Executable
    $start.Environment['KINOKO_TAS_PROFILE']='1';$start.Environment['KINOKO_TRACE']='0'
    foreach($arg in @('--save-dir',$saves,'--replay',$Replay,'--replay-status',$status,'--replay-identity',$identity,'--tas-dir',$bridge,'--tas-output',$output)){$start.ArgumentList.Add($arg)}
    $process=[Diagnostics.Process]::Start($start)
    try{
        Wait-State $process $bridge 0 1
        $watch=[Diagnostics.Stopwatch]::StartNew()
        Command $bridge "1 seek $frames`n"
        Wait-State $process $bridge 1 $frames
        $watch.Stop()
        $results += [pscustomobject]@{Iteration=$iteration;Frames=$frames-1;Milliseconds=$watch.Elapsed.TotalMilliseconds;FPS=($frames-1)/$watch.Elapsed.TotalSeconds}
        $results[-1] | Format-List | Out-String | Write-Output
        Command $bridge "2 stop 0`n"
        if(!$process.WaitForExit(15000)){throw 'Replay stop timed out'}
        if($process.ExitCode -ne 0){throw "Replay exit code $($process.ExitCode)"}
        if([IO.File]::ReadAllText($status) -notmatch "COMPLETED $frames frames"){throw 'Incomplete replay'}
        if((Get-FileHash -LiteralPath $Replay).Hash -ne (Get-FileHash -LiteralPath $output).Hash){throw 'Output differs from original replay'}
    }finally{
        if(!$process.HasExited){$process.Kill($true);$process.WaitForExit()}
        $process.Dispose()
    }
}
$results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'timings.json')
Get-FileHash -LiteralPath $Executable,$Replay | Format-List | Out-String | Set-Content -LiteralPath (Join-Path $OutputDirectory 'inputs.txt')
