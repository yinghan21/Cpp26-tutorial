param([Parameter(Mandatory = $true)][string]$Source)

# C++26 语言特性用 clang（Zig 自带）编译：需要 -std=c++26
# 用法： .\build-clang.ps1 samples\clang\12_cpp26.cpp

$zig = "C:\zig-0.17.0\zig.exe"
if (-not (Test-Path $zig)) { throw "找不到 zig：$zig" }

$src = (Resolve-Path $Source).Path
$name = [System.IO.Path]::GetFileNameWithoutExtension($src)
$build = Join-Path $PSScriptRoot "build"
New-Item -ItemType Directory -Force $build | Out-Null
$exe = Join-Path $build "$name-clang.exe"

Write-Host "[clang] $name"
& $zig c++ -std=c++26 -O2 -Wall $src -o $exe
if (-not (Test-Path $exe)) { throw "编译失败" }

& $exe
Write-Host "[ok] exit=$LASTEXITCODE"