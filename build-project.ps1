param(
    [Parameter(Position = 0, Mandatory = $true)][string]$Dir
,
    [switch]$NoRun,
    [switch]$Wall,
    [string]$ExtraArgs = ""
)

# Multi-file / module project build.
#   *.ixx  -> interface units; module name is auto-detected from `export module NAME;`
#   *.cpp  -> translation units, compiled and linked together
# Interface units are compiled in dependency order (repeated passes until no progress).
#
#   .\build-project.ps1 samples\proj_demo

$ErrorActionPreference = "Stop"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere not found: install Visual Studio with the Desktop C++ workload" }
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw "no Visual Studio installation with C++ tools found" }
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
$msvc = (Get-ChildItem (Join-Path $vsPath "VC\Tools\MSVC") -Directory |
         Sort-Object Name -Descending | Select-Object -First 1).FullName

$root = (Resolve-Path $Dir).Path
$projName = Split-Path $root -Leaf
$sysBuild = Join-Path $PSScriptRoot "build"
$build = Join-Path $sysBuild $projName
New-Item -ItemType Directory -Force $build | Out-Null

function Invoke-Cl([string]$CommandLine, [string]$WorkDir) {
    cmd /c "`"$vcvars`" >nul 2>&1 && cd /d `"$WorkDir`" && $CommandLine 2>&1"
}

# ---- 1. standard library module (once) ----
New-Item -ItemType Directory -Force $sysBuild | Out-Null
$stdIfc = Join-Path $sysBuild "std.ifc"
$stdObj = Join-Path $sysBuild "std.obj"
if (-not (Test-Path $stdIfc)) {
    Write-Host "[setup] building std.ifc (30-60s, once)"
    Invoke-Cl "cl /nologo /std:c++latest /EHsc /utf-8 /D_UTF8 /c `"$msvc\modules\std.ixx`" /ifcSearchDir `".`" /Fo:std.obj" $sysBuild | Out-Null
    if (-not (Test-Path $stdIfc)) { throw "std.ifc build failed" }
}
$compatObj = Join-Path $sysBuild "stdcompat.obj"

$warn = if ($Wall) { "/W4" } else { "/W3" }
$flags = "/nologo /std:c++latest /EHsc /utf-8 /O2 $warn /I`"$root`" $ExtraArgs"
$dirArgs = "/ifcSearchDir `".`" /ifcSearchDir `"$sysBuild`""

# ---- 2. interface units, dependency-ordered ----
$units = @(Get-ChildItem $root -Filter *.ixx | Sort-Object Name | ForEach-Object {
    $text = Get-Content -Raw -LiteralPath $_.FullName
    $moduleName = if ($text -match 'export\s+module\s+([A-Za-z_][\w.]*)\s*;') { $matches[1] } else { $_.BaseName }
    [pscustomobject]@{ File = $_; Module = $moduleName; Done = $false }
})

$built = @()
$round = 0
while ($true) {
    $round++
    $pending = @($units | Where-Object { -not $_.Done })
    if ($pending.Count -eq 0) { break }
    if ($round -gt 10) { throw "module build did not converge (circular imports?)" }

    $progress = $false
    foreach ($u in $pending) {
        $refStr = ($built | ForEach-Object { "/reference `"$_`"" }) -join " "
        Write-Host "[module] $($u.Module)"
        $out = Invoke-Cl "cl $flags /interface /c `"$($u.File.FullName)`" $refStr $dirArgs /Fo:`"$($u.Module).obj`"" $build
        if ($out | Where-Object { $_ -match ": error " }) {
            $out | Where-Object { $_ -match ": error " } | Select-Object -First 1 | ForEach-Object { Write-Host "  $_" }
        } else {
            $u.Done = $true
            $built += "$($u.Module)=$($u.Module).ifc"
            $progress = $true
        }
    }
    if (-not $progress) {
        $failed = ($pending | Where-Object { -not $_.Done } | ForEach-Object { $_.File.Name }) -join ', '
        throw "cannot compile these interface units: $failed"
    }
}

# ---- 3. translation units ----
$refArgs = ($built | ForEach-Object { "/reference `"$_`"" }) -join " "
$objs = @()
$cpp = @(Get-ChildItem $root -Filter *.cpp | Sort-Object Name)
foreach ($c in $cpp) {
    Write-Host "[compile] $($c.Name)"
    $out = Invoke-Cl "cl $flags /c `"$($c.FullName)`" $refArgs $dirArgs /Fo:`"$($c.BaseName).obj`"" $build
    $out | Where-Object { $_ -match ": error |: warning " }
    if ($out | Where-Object { $_ -match ": error " }) { throw "compile failed: $($c.Name)" }
    $objs += "$($c.BaseName).obj"
}
foreach ($u in $units) { $objs += "$($u.Module).obj" }

$needCompat = $false
foreach ($c in $cpp) { if ((Get-Content -Raw -LiteralPath $c.FullName) -match "import\s+std\.compat\s*;") { $needCompat = $true } }

$linkInputs = ($objs | ForEach-Object { "`"$_`"" }) -join " "
Write-Host "[link] $projName"
$exe = Join-Path $build "$projName.exe"
$extra = if ($needCompat) { " `"$compatObj`"" } else { "" }
$out = Invoke-Cl "link /nologo $linkInputs `"$stdObj`"$extra /OUT:`"$exe`"" $build
$out | Where-Object { $_ -match "error|warning" }

if (-not (Test-Path $exe)) { throw "compile failed" }
if ($NoRun) { Write-Host "[ok] $exe"; return }

cmd /c "`"$exe`" 2>&1"
Write-Host "[ok] exit=$LASTEXITCODE"
