# Portable Windows release for DokitLab Infant.
#
# Customer unpacks the folder and runs infant.exe.
# Layout next to exe (same idea as Linux pack-release.sh):
#   infant.exe, Qt/OpenSSL DLLs, assets/, data/, key/
#
# Prerequisites on BUILD machine:
#   - Qt 5.15+ MinGW with windeployqt / qmake
#   - Release build of infant
#
# Usage (PowerShell):
#   cd D:\projects\DokitLab\infant
#   .\tools\pack-windows.ps1
#   .\tools\pack-windows.ps1 -BuildDir ".\build\Desktop_Qt_5_15_2_MinGW_64_bit-Release" -QtBin "D:\Qt\5.15.2\mingw81_64\bin"
#
# Output: dist\Infant-Windows\  (+ Infant-Windows.zip)

param(
    [string]$BuildDir = "",
    [string]$QtBin = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $Root

function Find-BuildExe {
    param([string]$Preferred)
    $candidates = @()
    if ($Preferred) { $candidates += $Preferred }
    $candidates += @(
        (Join-Path $Root "build\Desktop_Qt_5_15_2_MinGW_64_bit-Release"),
        (Join-Path $Root "build\Desktop_Qt_5_15_2_MinGW_64_bit-Debug"),
        (Join-Path $Root "build-release"),
        (Join-Path $Root "build")
    )
    foreach ($dir in $candidates) {
        $exe = Join-Path $dir "infant.exe"
        if (Test-Path $exe) { return (Resolve-Path $dir).Path }
    }
    return $null
}

function Find-Windeployqt {
    param([string]$PreferredQtBin)
    if ($PreferredQtBin) {
        $w = Join-Path $PreferredQtBin "windeployqt.exe"
        if (Test-Path $w) { return $w }
    }
    $qmake = Get-Command qmake -ErrorAction SilentlyContinue
    if ($qmake) {
        $bin = Split-Path -Parent $qmake.Source
        $w = Join-Path $bin "windeployqt.exe"
        if (Test-Path $w) { return $w }
    }
    foreach ($g in @(
        "D:\Qt\5.15.2\mingw81_64\bin\windeployqt.exe",
        "C:\Qt\5.15.2\mingw81_64\bin\windeployqt.exe",
        "D:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe"
    )) {
        if (Test-Path $g) { return $g }
    }
    return $null
}

function Deploy-ByMapping {
    param(
        [string]$WindeployPath,
        [string]$ExePath,
        [string]$OutDir
    )
    $mapping = & $WindeployPath --list mapping --no-translations $ExePath 2>&1
    if ($LASTEXITCODE -ne 0 -and -not $mapping) {
        throw "windeployqt --list mapping failed"
    }
    $copied = 0
    foreach ($line in $mapping) {
        $text = "$line"
        if ($text -notmatch '^"(.+?)"\s+"(.+?)"$') { continue }
        $src = $Matches[1]
        $rel = $Matches[2]
        if ($rel -like "translations\*") { continue }
        if (-not (Test-Path -LiteralPath $src)) {
            Write-Warning "Missing source: $src"
            continue
        }
        $dest = Join-Path $OutDir $rel
        $destDir = Split-Path -Parent $dest
        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Force -Path $destDir | Out-Null
        }
        Copy-Item -LiteralPath $src -Destination $dest -Force
        $copied++
    }
    if ($copied -lt 5) {
        throw "Deploy-ByMapping copied too few files ($copied)"
    }
    Write-Host "Deployed $copied files via windeployqt mapping"
}

$BuildPath = Find-BuildExe -Preferred $BuildDir
if (-not $BuildPath) {
    Write-Error "infant.exe not found. Build the project first (Release recommended)."
}

$ExeSrc = Join-Path $BuildPath "infant.exe"
$AssetsSrc = Join-Path $BuildPath "assets"
if (-not (Test-Path $AssetsSrc)) {
    $AssetsSrc = Join-Path $Root "assets"
}
if (-not (Test-Path $AssetsSrc)) {
    Write-Error "assets folder not found near build or in project root."
}

$Windeploy = Find-Windeployqt -PreferredQtBin $QtBin
if (-not $Windeploy) {
    Write-Error "windeployqt.exe not found. Pass -QtBin path to Qt bin (e.g. D:\Qt\5.15.2\mingw81_64\bin)."
}

$qtBinDir = Split-Path -Parent $Windeploy
$qtRoot = Split-Path -Parent $qtBinDir
$env:QTDIR = $qtRoot
$env:PATH = ($qtBinDir + ";" + (Join-Path $qtRoot "..\..\Tools\mingw810_64\bin") + ";" + $env:PATH)

$DistRoot = Join-Path $Root "dist"
$OutDir = Join-Path $DistRoot "Infant-Windows"
if (Test-Path $OutDir) {
    try {
        Remove-Item -Recurse -Force $OutDir -ErrorAction Stop
    } catch {
        $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
        $OutDir = Join-Path $DistRoot "Infant-Windows-$stamp"
        Write-Warning "Old dist folder locked; writing to $OutDir"
    }
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Write-Host "Copying infant.exe ..."
Copy-Item $ExeSrc (Join-Path $OutDir "infant.exe") -Force

Write-Host "Copying assets ..."
Copy-Item $AssetsSrc (Join-Path $OutDir "assets") -Recurse -Force
# Guard against nested assets/assets from bad previous copies in source
$nested = Join-Path $OutDir "assets\assets"
if (Test-Path $nested) {
    Remove-Item -Recurse -Force $nested
}

Write-Host "Creating data/ and key/ ..."
New-Item -ItemType Directory -Force -Path (Join-Path $OutDir "data") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $OutDir "data\scans") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $OutDir "key") | Out-Null

$sslNames = @("libcrypto-1_1-x64.dll", "libssl-1_1-x64.dll")
foreach ($dll in $sslNames) {
    $fromBuild = Join-Path $BuildPath $dll
    if (Test-Path $fromBuild) {
        Copy-Item $fromBuild $OutDir -Force
    }
}
$missingSsl = @($sslNames | Where-Object { -not (Test-Path (Join-Path $OutDir $_)) })
if ($missingSsl.Count -gt 0) {
    foreach ($root in @(
        "D:\Qt\Tools\mingw1120_64\opt\bin",
        "D:\Qt\Tools\mingw1310_64\opt\bin",
        "D:\Qt\Tools\mingw810_64\opt\bin",
        "C:\Qt\Tools\mingw1120_64\opt\bin"
    )) {
        foreach ($dll in $missingSsl) {
            $p = Join-Path $root $dll
            if (Test-Path $p) { Copy-Item $p $OutDir -Force }
        }
        $missingSsl = @($sslNames | Where-Object { -not (Test-Path (Join-Path $OutDir $_)) })
        if ($missingSsl.Count -eq 0) { break }
    }
}
if ($missingSsl.Count -gt 0) {
    Write-Warning ("OpenSSL DLLs missing: {0}. HTTPS/license activation will fail." -f ($missingSsl -join ", "))
}

Write-Host "Deploying Qt runtime ..."
$exePath = Join-Path $OutDir "infant.exe"
$deployOk = $false
try {
    $deployArgs = @("--no-translations", "--no-system-d3d-compiler", "--compiler-runtime", $exePath)
    if ($BuildPath -match "Release") { $deployArgs = @("--release") + $deployArgs }
    elseif ($BuildPath -match "Debug") { $deployArgs = @("--debug") + $deployArgs }
    & $Windeploy @deployArgs
    if ($LASTEXITCODE -eq 0 -and (Test-Path (Join-Path $OutDir "platforms\qwindows.dll"))) {
        $deployOk = $true
        Write-Host "windeployqt OK"
    }
} catch {
    Write-Warning $_.Exception.Message
}

if (-not $deployOk) {
    Write-Warning "windeployqt copy failed — falling back to --list mapping"
    Deploy-ByMapping -WindeployPath $Windeploy -ExePath $exePath -OutDir $OutDir
}

# Ensure MinGW runtime
$mingwBins = @(
    "D:\Qt\Tools\mingw810_64\bin",
    "D:\Qt\Tools\mingw1120_64\bin",
    "D:\Qt\Tools\mingw1310_64\bin",
    "C:\Qt\Tools\mingw810_64\bin"
) | Where-Object { Test-Path $_ }
foreach ($dll in @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")) {
    if (Test-Path (Join-Path $OutDir $dll)) { continue }
    foreach ($bin in $mingwBins) {
        $p = Join-Path $bin $dll
        if (Test-Path $p) {
            Copy-Item $p $OutDir -Force
            break
        }
    }
}

@"
[Paths]
Prefix=.
Plugins=.
Libraries=.
"@ | Set-Content -Path (Join-Path $OutDir "qt.conf") -Encoding ASCII

$Readme = @"
DokitLab Infant — Windows

Запуск: infant.exe

Рядом с программой:
  assets/     — ресурсы упражнений и протоколов
  data/       — база SQLite (base.db) и сканы (scans/)
  key/        — файл лицензии (license.json)

Режим двух экранов: Администрирование → «Два экрана»
  (настройка сохраняется в data/настройки.txt)

Не устанавливайте в Program Files без прав записи —
папки data/ и key/ должны быть доступны пользователю на запись.

Для HTTPS нужны libcrypto-1_1-x64.dll и libssl-1_1-x64.dll
(обычно уже лежат рядом с infant.exe).
"@
Set-Content -Path (Join-Path $OutDir "README.txt") -Value $Readme -Encoding UTF8

# Critical checks
$required = @(
    "infant.exe",
    "Qt5Core.dll",
    "platforms\qwindows.dll",
    "sqldrivers\qsqlite.dll",
    "assets",
    "data",
    "key",
    "qt.conf"
)
$bad = @()
foreach ($r in $required) {
    if (-not (Test-Path (Join-Path $OutDir $r))) { $bad += $r }
}
if ($bad.Count -gt 0) {
    Write-Error ("Dist incomplete, missing: {0}" -f ($bad -join ", "))
}

$zipPath = Join-Path $DistRoot "Infant-Windows.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Write-Host "Creating zip ..."
Compress-Archive -Path $OutDir -DestinationPath $zipPath -Force

$sizeMb = [math]::Round(((Get-ChildItem $OutDir -Recurse -File | Measure-Object Length -Sum).Sum) / 1MB, 1)
Write-Host ""
Write-Host "OK: $OutDir  ($sizeMb MB)"
Write-Host "ZIP: $zipPath"
