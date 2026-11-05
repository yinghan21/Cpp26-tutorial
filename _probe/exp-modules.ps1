# 实验：std.compat 构建 / 自写模块 / 头文件单元 / 消除 C5050 警告
$ErrorActionPreference = "Stop"
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
$msvc = (Get-ChildItem (Join-Path $vsPath "VC\Tools\MSVC") -Directory | Sort-Object Name -Descending | Select-Object -First 1).FullName
$dir = Join-Path $PSScriptRoot "mod"
New-Item -ItemType Directory -Force $dir | Out-Null
$utf8 = [System.Text.UTF8Encoding]::new($false)

function Run-Cl([string]$cmd) { cmd /c "`"$vcvars`" >nul 2>&1 && cd /d `"$dir`" && $cmd 2>&1" }

# --- 1) 用 /D_UTF8 重建 std.ifc，看 C5050 是否消失 ---
Write-Host "=== 1. 重建 std.ifc（带 /D_UTF8）==="
Run-Cl "cl /nologo /std:c++latest /EHsc /utf-8 /D_UTF8 /c `"$msvc\modules\std.ixx`" /ifcSearchDir `".`" /Fo:std.obj" | Select-Object -First 5
Get-ChildItem $dir -Filter *.ifc | Select-Object Name, Length

# --- 2) std.compat ---
Write-Host "=== 2. std.compat ==="
Run-Cl "cl /nologo /std:c++latest /EHsc /utf-8 /D_UTF8 /c `"$msvc\modules\std.compat.ixx`" /ifcSearchDir `".`" /Fo:stdcompat.obj" | Select-Object -First 5
Get-ChildItem $dir -Filter *.ifc | Select-Object Name, Length

# --- 3) 自写模块 ---
Write-Host "=== 3. 自写模块 ==="
[System.IO.File]::WriteAllText("$dir\geo.ixx", @'
export module geo;

export namespace geo {
    int add(int a, int b) { return a + b; }
}
'@, $utf8)

[System.IO.File]::WriteAllText("$dir\modmain.cpp", @'
import std;
import geo;

int main() {
    std::println("add={}", geo::add(2, 3));
}
'@, $utf8)

Run-Cl "cl /nologo /std:c++latest /EHsc /utf-8 /interface /c geo.ixx /ifcSearchDir `".`" /Fo:geo.obj" | Select-Object -First 5
Run-Cl "cl /nologo /std:c++latest /EHsc /utf-8 /c modmain.cpp /reference `"geo=geo.ifc`" /ifcSearchDir `".`" /Fo:modmain.obj" | Select-Object -First 8
Run-Cl "link /nologo modmain.obj geo.obj std.obj /OUT:modmain.exe" | Select-Object -First 5
& "$dir\modmain.exe"

# --- 4) 头文件单元 import "x.h" ---
Write-Host "=== 4. 头文件单元 ==="
[System.IO.File]::WriteAllText("$dir\legacy.h", @'
#pragma once
inline int legacy_twice(int x) { return x * 2; }
'@, $utf8)
[System.IO.File]::WriteAllText("$dir\hunit.cpp", @'
import std;
import "legacy.h";

int main() {
    std::println("twice={}", legacy_twice(21));
}
'@, $utf8)
Run-Cl "cl /nologo /std:c++latest /EHsc /utf-8 /c hunit.cpp /ifcSearchDir `".`" /Fo:hunit.obj" | Select-Object -First 8
