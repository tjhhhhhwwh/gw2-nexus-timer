param([string]$GameDirectory = "", [string]$GitHubRepository = "tjhhhhhwwh/gw2-nexus-timer", [string]$Generator = "")
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
Push-Location $Root
try {
    python tools/generate_data.py --check
    if ($LASTEXITCODE -ne 0) { throw "Schedule data check failed" }
    python tools/generate_events.py --check
    if ($LASTEXITCODE -ne 0) { throw "Event data check failed" }
    if (-not $Generator) {
        $VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio/Installer/vswhere.exe"
        $VsMajor = 17
        if (Test-Path $VsWhere) {
            $VsVersion = & $VsWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
            if ($VsVersion) { $VsMajor = ([version]$VsVersion).Major }
        }
        $Generator = if ($VsMajor -ge 18) { "Visual Studio 18 2026" } else { "Visual Studio 17 2022" }
    }
    cmake -S . -B build/windows -G $Generator -A x64 "-DGW2_TIMER_GITHUB_REPOSITORY=$GitHubRepository"
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
    cmake --build build/windows --config Release
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
    ctest --test-dir build/windows -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
    New-Item -ItemType Directory -Force dist | Out-Null
    Copy-Item build/windows/Release/gw2_nexus_timer.dll dist/gw2_nexus_timer.dll
    Get-FileHash dist/gw2_nexus_timer.dll -Algorithm SHA256
    if ($GameDirectory) {
        if (-not (Test-Path (Join-Path $GameDirectory "Gw2-64.exe"))) { throw "Not a GW2 game directory" }
        $Destination = Join-Path $GameDirectory "addons"
        New-Item -ItemType Directory -Force $Destination | Out-Null
        Copy-Item dist/gw2_nexus_timer.dll (Join-Path $Destination "gw2_nexus_timer.dll")
    }
} finally {
    Pop-Location
}
