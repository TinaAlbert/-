param([string]$Executable = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (!$Executable) { $Executable = Join-Path $root 'build/graphics-lab.exe' }
$outputDir = Join-Path $root 'docs/images'
$tempDir = Join-Path $root '.cache/previews'
New-Item -ItemType Directory -Force -Path $outputDir,$tempDir | Out-Null
Add-Type -AssemblyName System.Drawing
$previousVideo = $env:SDL_VIDEODRIVER
try {
    $env:SDL_VIDEODRIVER = 'dummy'
    $names = @('shapes','pattern-fill','cube','bezier')
    for ($i = 0; $i -lt $names.Count; ++$i) {
        $bmp = Join-Path $tempDir "$($names[$i]).bmp"
        & $Executable --scene ($i + 1) --export-demo $bmp
        if ($LASTEXITCODE -ne 0) { throw "Scene $($i + 1) export failed." }
        $bitmap = [System.Drawing.Image]::FromFile($bmp)
        try {
            $bitmap.Save((Join-Path $outputDir "$($names[$i]).png"), [System.Drawing.Imaging.ImageFormat]::Png)
        } finally { $bitmap.Dispose() }
    }
} finally { $env:SDL_VIDEODRIVER = $previousVideo }
