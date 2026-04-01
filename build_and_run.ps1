# If execution policy blocks the script, run once as admin:
#   Set-ExecutionPolicy -Scope CurrentUser RemoteSigned

$ErrorActionPreference = "Stop"

function Info    { param($msg) Write-Host "[INFO]  $msg" -ForegroundColor Cyan    }
function Success { param($msg) Write-Host "[OK]    $msg" -ForegroundColor Green   }
function Warn    { param($msg) Write-Host "[WARN]  $msg" -ForegroundColor Yellow  }
function Fail    { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red; exit 1 }

function Check-Tool {
    param([string]$Cmd, [string]$Hint)
    if (-not (Get-Command $Cmd -ErrorAction SilentlyContinue)) {
        Fail "'$Cmd' not found. $Hint"
    }
    Success "$Cmd found: $((Get-Command $Cmd).Source)"
}

Info "Checking prerequisites..."
Check-Tool "cmake" "Install from https://cmake.org"
Check-Tool "ninja" "Install from https://ninja-build.org  (or 'winget install Ninja-build.Ninja')"
Check-Tool "vcpkg" "Install from https://vcpkg.io/en and make sure it is on your PATH"

Info "Locating vcpkg toolchain file..."

$Toolchain = $null

if ($env:VCPKG_ROOT -and (Test-Path "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake")) {
    $Toolchain = "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
}

if (-not $Toolchain) {
    $VcpkgBin = (Get-Command vcpkg -ErrorAction SilentlyContinue).Source
    if ($VcpkgBin) {
        $VcpkgDir = Split-Path $VcpkgBin -Parent
        $Candidate = Join-Path $VcpkgDir "scripts\buildsystems\vcpkg.cmake"
        if (Test-Path $Candidate) { $Toolchain = $Candidate }
    }
}

if (-not $Toolchain) {
    $IntegrateOut = & vcpkg integrate install 2>&1 | Out-String
    $Match = [regex]::Match($IntegrateOut, '[^\s]+vcpkg\.cmake')
    if ($Match.Success -and (Test-Path $Match.Value)) {
        $Toolchain = $Match.Value
    }
}

if (-not $Toolchain) {
    Warn "Could not auto-detect the vcpkg toolchain file."
    $Toolchain = Read-Host "Please enter the full path to vcpkg.cmake"
}

if (-not (Test-Path $Toolchain)) {
    Fail "Toolchain file not found at: $Toolchain"
}
Success "vcpkg toolchain: $Toolchain"

Info "Installing vcpkg dependencies..."

if (Test-Path "vcpkg.json") {
    Info "Found vcpkg.json — installing all manifest dependencies..."
    & vcpkg install
    if ($LASTEXITCODE -ne 0) { Fail "vcpkg install failed." }
} else {
    $VcpkgPackages = @(
        "glfw3",
        "glm",
        "imgui",
        "tinyobjloader",
        "opengl"
    )
    Warn "No vcpkg.json found. Installing common packages: $($VcpkgPackages -join ', ')"
    & vcpkg install @VcpkgPackages
    if ($LASTEXITCODE -ne 0) { Fail "vcpkg install failed." }
}

Success "vcpkg dependencies installed."

Info "Configuring project with CMake..."

& cmake -B build -G "Ninja" "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { Fail "CMake configuration failed." }

Success "CMake configuration complete."

Info "Building project..."
& cmake --build build --config Release
if ($LASTEXITCODE -ne 0) { Fail "Build failed." }

Success "Build complete."

Info "Searching for executable..."

$Candidates = @(
    ".\build\3d-renderer.exe",
    ".\build\renderer.exe",
    ".\build\raytracer.exe",
    ".\build\Release\3d-renderer.exe",
    ".\build\Release\renderer.exe",
    ".\build\Release\raytracer.exe",
    ".\build\3d-renderer",
    ".\build\renderer",
    ".\build\raytracer"
)

$Executable = $null
foreach ($c in $Candidates) {
    if (Test-Path $c) { $Executable = $c; break }
}

if (-not $Executable) {
    $Found = Get-ChildItem -Path ".\build" -Recurse -Depth 2 -Filter "*.exe" `
             | Where-Object { $_.Name -notmatch '\.(cmake|sh)$' } `
             | Select-Object -First 1
    if ($Found) { $Executable = $Found.FullName }
}

if (-not $Executable) {
    Warn "Could not automatically locate the built executable."
    Warn "Look inside .\build\ and run it manually."
    exit 0
}

Success "Found executable: $Executable"
Info "Launching 3D Raytracer..."
& $Executable