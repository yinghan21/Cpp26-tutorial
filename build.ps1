param(
    [Parameter(Mandatory = $true)][string]$Source
)

# 一次性生成 std 模块（std.ifc）+ 编译运行一个 .cpp
# 用法： .\build.ps1 samples\hello.cpp

$ErrorActionPreference = "Stop"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "找不到 vswhere，先装 Visual Studio（含 C++ 桌面开发）" }
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw "找不到带 C++ 工具的 Visual Studio" }

$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
$msvcRoot = (Get-ChildItem (Join-Path $vsPath "VC\Tools\MSVC") -Directory |
             Sort-Object Name -Descending | Select-Object -First 1).FullName

$build = Join-Path $PSScriptRoot "build"
New-Item -ItemType Directory -Force $build | Out-Null

$stdIfc = Join-Path $build "std.ifc"
$stdObj = Join-Path $build "std.obj"

# 1) 标准库模块只需要生成一次
if (-not (Test-Path $stdIfc)) {
    Write-Host "[setup] 生成 std.ifc（大约 30 秒，只需一次）"
    cmd /c "cd /d `"$build`" && `"$vcvars`" >nul && cl /nologo /std:c++latest /EHsc /utf-8 /c `"$msvcRoot\modules\std.ixx`" /ifcSearchDir `"$build`" /Fo:`"$stdObj`""
    if (-not (Test-Path $stdIfc)) { throw "std.ifc 生成失败" }
}

$src = (Resolve-Path $Source).Path
$name = [System.IO.Path]::GetFileNameWithoutExtension($src)
$obj = Join-Path $build "$name.obj"
$exe = Join-Path $build "$name.exe"

# 2) 编译 + 链接
Write-Host "[build] $name"
cmd /c "`"$vcvars`" >nul && cl /nologo /std:c++latest /EHsc /utf-8 /O2 /c `"$src`" /ifcSearchDir `"$build`" /Fo:`"$obj`" && link /nologo `"$obj`" `"$stdObj`" /OUT:`"$exe`""
if (-not (Test-Path $exe)) { throw "编译失败" }

# 3) 运行
& $exe
