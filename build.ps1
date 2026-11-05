param(
    [Parameter(Position = 0, Mandatory = $true)][string]$Source
,
    [switch]$NoRun,
    [switch]$Wall,
    [string]$ExtraArgs = ""
)

# Single-file build for samples that use `import std;`
#   .\build.ps1 samples\01_hello.cpp
#   .\build.ps1 samples\01_hello.cpp -Wall    # /W4
#   .\build.ps1 samples\01_hello.cpp -NoRun   # compile only
#
# First run generates build\std.ifc and (on demand) build\std.compat.ifc.

$ErrorActionPreference = "Stop"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere not found: install Visual Studio with the Desktop C++ workload" }
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw "no Visual Studio installation with C++ tools found" }
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
$msvc = (Get-ChildItem (Join-Path $vsPath "VC\Tools\MSVC") -Directory |
         Sort-Object Name -Descending | Select-Object -First 1).FullName

$build = Join-Path $PSScriptRoot "build"
New-Item -ItemType Directory -Force $build | Out-Null

function Invoke-Cl([string]$CommandLine) {
    cmd /c "`"$vcvars`" >nul 2>&1 && cd /d `"$build`" && $CommandLine 2>&1"
}

# ---- 1. standard library modules (built once) ----
$stdIfc = Join-Path $build "std.ifc"
$stdObj = Join-Path $build "std.obj"
if (-not (Test-Path $stdIfc)) {
    Write-Host "[setup] building std.ifc (30-60s, once)"
    Invoke-Cl "cl /nologo /std:c++latest /EHsc /utf-8 /D_UTF8 /c `"$msvc\modules\std.ixx`" /ifcSearchDir `".`" /Fo:std.obj" | Out-Null
    if (-not (Test-Path $stdIfc)) { throw "std.ifc build failed" }
}

$compatIfc = Join-Path $build "std.compat.ifc"
$compatObj = Join-Path $build "stdcompat.obj"
$srcText = Get-Content -Raw -LiteralPath $Source
$needCompat = $srcText -match "import\s+std\.compat\s*;"
if ($needCompat -and -not (Test-Path $compatIfc)) {
    Write-Host "[setup] building std.compat.ifc (20s, once)"
    Invoke-Cl "cl /nologo /std:c++latest /EHsc /utf-8 /D_UTF8 /c `"$msvc\modules\std.compat.ixx`" /ifcSearchDir `".`" /Fo:stdcompat.obj" | Out-Null
    if (-not (Test-Path $compatIfc)) { throw "std.compat.ifc build failed" }
}

# ---- 2. compile + link ----
$src = (Resolve-Path $Source).Path
$name = [System.IO.Path]::GetFileNameWithoutExtension($src)
$obj = Join-Path $build "$name.obj"
$exe = Join-Path $build "$name.exe"
if (Test-Path $exe) { Remove-Item -LiteralPath $exe -Force }
if (Test-Path $obj) { Remove-Item -LiteralPath $obj -Force }

$warn = if ($Wall) { "/W4" } else { "/W3" }
$flags = "/nologo /std:c++latest /EHsc /utf-8 /O2 $warn $ExtraArgs"

Write-Host "[build] $name"
$out = Invoke-Cl "cl $flags /c `"$src`" /ifcSearchDir `".`" /Fo:`"$obj`""
$out | Where-Object { $_ -match "error|warning|\.cpp" }
if ($out | Where-Object { $_ -match ": error " }) { throw "compile failed ($name)" }
if (-not (Test-Path $obj)) { throw "compile failed: no object file ($name)" }

$linkInputs = "`"$obj`" `"$stdObj`""
if ($needCompat) { $linkInputs += " `"$compatObj`"" }
$linkOut = Invoke-Cl "link /nologo $linkInputs /OUT:`"$exe`""
$linkOut | Where-Object { $_ -match "error|warning" }
if (-not (Test-Path $exe)) { throw "link failed ($name)" }
if ($NoRun) { Write-Host "[ok] $exe"; return }

# ---- 3. run ----
cmd /c "`"$exe`" 2>&1"
Write-Host "[ok] exit=$LASTEXITCODE"
