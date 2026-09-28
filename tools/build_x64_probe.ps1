param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9][A-Za-z0-9_-]*$')][string]$Name,
    [Parameter(Mandatory=$true)][string]$SourceDir,
    [string]$Generator = 'Visual Studio 17 2022'
)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$assets=(Resolve-Path -LiteralPath $SourceDir).Path
$buildTree=Join-Path $repo "build-runs/$Name"
$runDirectory=Join-Path $repo "runtime-builds/$Name"
foreach($path in @($buildTree,$runDirectory)) {
    if(Test-Path -LiteralPath $path) { throw "Existing artifacts are retained: $path" }
}
$revision=(& git -C $repo rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0) { throw 'Cannot identify source revision' }
$changes=& git -C $repo status --porcelain --untracked-files=no
if($LASTEXITCODE -ne 0 -or $changes) { throw 'Commit tracked changes before recording a build' }
foreach($letter in @('a','b','c')) {
    if(-not(Test-Path -LiteralPath (Join-Path $assets "6kinoko_$letter.dat"))) { throw 'Missing reference DAT' }
}
New-Item -ItemType Directory -Path $buildTree | Out-Null
$revision | Set-Content -LiteralPath (Join-Path $buildTree 'source-commit.txt') -Encoding ascii
# Native stderr is diagnostic data; the exit code decides success.
$ErrorActionPreference='Continue'
& cmake -S $repo -B $buildTree -A x64 -G $Generator -DKINOKO_EXPERIMENTAL_X64=ON `
    -DKINOKO_RETDEC_DISABLE_TRACE=ON "-DKINOKO_REFERENCE_DIR=$assets" "-DKINOKO_RUNTIME_DIR=$runDirectory" *> (Join-Path $buildTree 'configure.log')
$configureExit=$LASTEXITCODE
$ErrorActionPreference='Stop'
$buildExit=$null
$staged=$false
if($configureExit -eq 0) {
    $ErrorActionPreference='Continue'
    & cmake --build $buildTree --config Release --target kinoko_modern_gpu --parallel 4 *> (Join-Path $buildTree 'build.log')
    $buildExit=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if($buildExit -eq 0) {
        & (Join-Path $PSScriptRoot 'stage_dat.ps1') -Executable (Join-Path $runDirectory 'kinoko_modern_gpu.exe') -SourceDir $assets *> (Join-Path $buildTree 'dat.log')
        $staged=$true
    }
}
$record=[ordered]@{
    source_commit=$revision; configuration='Windows x64 Release'; target='kinoko_modern_gpu'
    configure_exit=$configureExit; build_exit=$buildExit
    full_build_succeeded=($configureExit -eq 0 -and $null -ne $buildExit -and $buildExit -eq 0)
    dat_staging_verified=$staged; game_run=$false; tests_run=$false
    source_guards_enabled=$true; build_directory=$buildTree; runtime_directory=$runDirectory
}
$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $buildTree 'artifacts.json') -Encoding utf8
Write-Output "MANIFEST $buildTree/artifacts.json"
if(-not $record.full_build_succeeded) { throw "Full x64 game remains blocked; see retained configure/build logs in $buildTree" }
