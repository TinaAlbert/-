param(
    [string]$Compiler = 'g++',
    [string]$Proxy = '',
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$deps = Join-Path $root '.deps'
$build = Join-Path $root 'build'
$tempDir = Join-Path $root '.cache/tmp'
New-Item -ItemType Directory -Force -Path $deps,$build,$tempDir | Out-Null
$previousTemp = $env:TEMP
$previousTmp = $env:TMP
$env:TEMP = $tempDir
$env:TMP = $tempDir
try {
    $compilerPath = (Get-Command $Compiler -ErrorAction Stop).Source
    $target = & $compilerPath -dumpmachine
    if ($LASTEXITCODE -ne 0 -or $target -notmatch '^x86_64.*mingw') {
        throw 'This script requires a 64-bit MinGW-w64 g++ compiler. For MSVC use CMake.'
    }
    $version = '2.32.6'
    $archive = Join-Path $deps "SDL2-devel-$version-mingw.tar.gz"
    $expectedHash = '2C5EF8CF20491649F7726D76BBFE487FFDD83E2A3A594C37734AC18BDDDFEC6B'
    $sdl = Join-Path $deps "SDL2-$version/x86_64-w64-mingw32"
    if (!(Test-Path -LiteralPath "$sdl/include/SDL2/SDL.h")) {
        if (!(Test-Path -LiteralPath $archive)) {
            $download = @{
                Uri = "https://github.com/libsdl-org/SDL/releases/download/release-$version/SDL2-devel-$version-mingw.tar.gz"
                OutFile = "$archive.download"
                UseBasicParsing = $true
            }
            if ($Proxy) { $download.Proxy = $Proxy }
            Invoke-WebRequest @download
            if ((Get-FileHash -LiteralPath "$archive.download" -Algorithm SHA256).Hash -ne $expectedHash) {
                throw 'SDL2 archive checksum mismatch.'
            }
            Move-Item -LiteralPath "$archive.download" -Destination $archive -Force
        }
        if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expectedHash) {
            throw 'SDL2 archive checksum mismatch.'
        }
        & tar -xzf $archive -C $deps
        if ($LASTEXITCODE -ne 0) { throw 'SDL2 extraction failed.' }
    }
    & $compilerPath -std=c++17 -O2 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 `
        -I "$sdl/include/SDL2" "$root/src/main.cpp" -L "$sdl/lib" `
        -lSDL2 -lcomdlg32 -static-libgcc -static-libstdc++ -o "$build/graphics-lab.exe"
    if ($LASTEXITCODE -ne 0) { throw 'Application build failed.' }
    Copy-Item -LiteralPath "$sdl/bin/SDL2.dll" -Destination $build -Force
    # Some MinGW distributions dynamically link winpthreads even with static libstdc++.
    $pthread = Join-Path (Split-Path -Parent $compilerPath) 'libwinpthread-1.dll'
    if (Test-Path -LiteralPath $pthread) { Copy-Item -LiteralPath $pthread -Destination $build -Force }
    Copy-Item -LiteralPath "$root/THIRD_PARTY_NOTICES.md" -Destination $build -Force
    if ($Test) {
        & $compilerPath -std=c++17 -O2 -Wall -Wextra -Wpedantic "$root/tests/graphics_tests.cpp" `
            -static-libgcc -static-libstdc++ -o "$build/graphics-tests.exe"
        if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
        & "$build/graphics-tests.exe"
        if ($LASTEXITCODE -ne 0) { throw 'Graphics regression tests failed.' }
        $previousVideo = $env:SDL_VIDEODRIVER
        try {
            $env:SDL_VIDEODRIVER = 'dummy'
            foreach ($scene in 1..4) {
                & "$build/graphics-lab.exe" --scene $scene --smoke-test
                if ($LASTEXITCODE -ne 0) { throw "Scene $scene smoke test failed." }
            }
        } finally { $env:SDL_VIDEODRIVER = $previousVideo }
    }
    Write-Host "Built: $build/graphics-lab.exe"
} finally {
    $env:TEMP = $previousTemp
    $env:TMP = $previousTmp
}
