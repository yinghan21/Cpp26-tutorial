---
title: CMake 与现代工程结构
tags: [cpp, cpp26, 工程]
chapter: 5
order: 37
created: 2026-11-05
---

# CMake 与现代工程结构

> [!info] 本节目标
> - 知道一个 C++ 项目应该长什么样（目录结构）
> - 会写**现代写法**的 `CMakeLists.txt`（`target_*` 一派）
> - 知道 MSVC 下要加的编译选项
> - 了解 CMake 对 C++ 模块的支持现状
> - 明白"没有 CMake 时"如何等价验证

> [!warning] 本机环境说明
> 本机**没有安装 cmake 与 ninja**（`cmake --version` 报 not found）。所以本章的 CMake 内容是**配置示范**，没有运行验证；
> 已验证的是目录结构与本教程的 `build-project.ps1` 流程。想跑起来先装：`winget install Kitware.CMake`，再 `cmake --version` 确认。

> [!tip] 先看结论
> - **一个目标（target）一套属性**：`target_sources` / `target_include_directories` / `target_compile_options`。
> - 不要再用全局的 `include_directories()` / `link_libraries()`（它们会传染给所有目标）。
> - MSVC 必加 `/utf-8`（否则中文乱码）、`/W4`（警告）、`/std:c++latest`（模块）。
> - C++ 模块在 CMake 里需要 **3.28+** 且要显式声明 `CXX_MODULES` 文件集。

## 一、目录结构

```text
my_project/
├── CMakeLists.txt
├── CMakePresets.json        # 可选：预设构建配置
├── src/
│   ├── main.cpp
│   ├── core.ixx             # 模块接口
│   └── core.cpp             # 模块实现（可选）
├── include/                 # 传统头文件（如果有）
│   └── mylib/geo.h
├── tests/
│   └── test_geo.cpp
├── third_party/             # 第三方（或交给 FetchContent/vcpkg）
└── build/                   # 构建产物（不进版本控制）
```

| 约定 | 理由 |
| --- | --- |
| `src/` 与 `include/` 分离 | 区分"内部实现"与"对外接口" |
| `build/` 独立 | 支持多配置（Debug/Release）并存 |
| 模块文件放 `src/` | 与源文件同级，便于 CMake 扫描 |
| 测试独立目录 | 方便单独构建测试目标 |

## 二、最小可用 `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # 关掉编译器扩展，严格标准

add_executable(app
    src/main.cpp
)

target_include_directories(app PRIVATE include)

# MSVC 专用选项
target_compile_options(app PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /W4 /permissive- /std:c++latest>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
```

逐段解释：

| 行 | 作用 |
| --- | --- |
| `cmake_minimum_required(VERSION 3.28)` | 模块支持需要较新版本 |
| `project(... LANGUAGES CXX)` | 只启用 C++，加快配置 |
| `CMAKE_CXX_STANDARD 23` | 默认标准（模块还需要 `/std:c++latest`） |
| `add_executable(app ...)` | 声明目标 |
| `target_include_directories(app PRIVATE ...)` | 只给 `app` 用 |
| `$<$<CXX_COMPILER_ID:MSVC>:...>` | 生成器表达式：按编译器选选项 |

## 三、现代写法 vs 老写法

| 老写法（别用） | 现代写法 |
| --- | --- |
| `include_directories(include)` | `target_include_directories(t PRIVATE include)` |
| `link_libraries(foo)` | `target_link_libraries(t PRIVATE foo)` |
| `add_definitions(-DFOO)` | `target_compile_definitions(t PRIVATE FOO)` |
| `set(CMAKE_CXX_FLAGS "...")` | `target_compile_options(t PRIVATE ...)` |
| `file(GLOB ...)` | 显式列出源文件（GLOB 不会自动感知新增文件） |

为什么：**全局设置会传染**。A 目标需要 `-DFOO`，结果 B、C 全带上了，出问题极难排查。

## 四、库 + 可执行文件

```cmake
add_library(core STATIC
    src/core.cpp
    src/geo.cpp
)
target_include_directories(core PUBLIC include)     # PUBLIC：使用者也能拿到

add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE core)
```

| 关键字 | 含义 |
| --- | --- |
| `PRIVATE` | 只本目标用 |
| `PUBLIC` | 本目标用 + 传给链接它的目标 |
| `INTERFACE` | 本目标不用，只传给使用者（头文件库常用） |

## 五、C++ 模块（3.28+）

```cmake
add_library(core)
target_sources(core
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/core.ixx
    PRIVATE
        src/core_impl.cpp
)
```

现状（2026 年）：

| 工具链 | 模块支持 |
| --- | --- |
| MSVC 19.28+ | ✅ 完整（`/interface` / `/reference`） |
| CMake 3.28+ | ✅ `CXX_MODULES` 文件集 |
| Ninja 1.11+ | ✅ 需要 `ninja --version` 支持 dyndep |
| GCC 14+ | ⚠️ 部分 |
| Clang 17+ | ⚠️ 需要自己构建 `std` 模块 |

## 六、Debug / Release 与多配置

```cmake
# 单配置生成器（Ninja / Makefiles）
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug

# 多配置生成器（Visual Studio）
cmake -S . -B build -G "Visual Studio 18 2026"
cmake --build build --config Release
```

| 配置 | 优化 | 调试信息 |
| --- | --- | --- |
| Debug | `Od` | `/Zi` |
| Release | `O2` | 无 |
| RelWithDebInfo | `O2` | `/Zi` |

## 七、第三方库

```cmake
include(FetchContent)
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.0.0
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(app PRIVATE fmt::fmt)
```

或者用 vcpkg 清单模式（推荐给团队）：

```json
{ "dependencies": ["fmt", "nlohmann-json"] }
```

## 八、没有 CMake 时的等价验证

本教程全部示例用这两个脚本，效果等价于"编译一个目标"：

| 需求 | 本教程做法 |
| --- | --- |
| 单文件程序 | `.\build.ps1 samples\xx.cpp` |
| 多文件/模块工程 | `.\build-project.ps1 samples\proj_xx` |
| 打开警告 | `-Wall`（即 `/W4`） |
| Debug 构建 | 手动加 `-ExtraArgs "/Od /Zi"` |

所以：**先用脚本把程序写对，再换成 CMake**。构建系统只是把同样的命令行参数组织起来。

## 示例：本章对应的 `CMakeLists.txt`

`samples\proj_cmake\CMakeLists.txt`（配置示范，本机未运行）：

```cmake
cmake_minimum_required(VERSION 3.28)
project(proj_cmake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(proj_cmake main.cpp)

target_compile_options(proj_cmake PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /W4 /permissive- /std:c++latest>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -std=c++23>
)
```

同一份 `main.cpp` 用本教程脚本验证：

```powershell
.\build.ps1 samples\proj_cmake\main.cpp
```

## 常见错误与排查

| 现象 | 报错 | 原因 | 改法 |
| --- | --- | --- | --- |
| 找不到头文件 | `C1083` | 没 `target_include_directories` | 补上，注意 `PUBLIC`/`PRIVATE` |
| 中文乱码 | 无 | 缺 `/utf-8` | 加编译选项 |
| 模块找不到 | `C2230` | CMake 没开 `CXX_MODULES` | 用 3.28+ 并声明文件集 |
| 改了源码没重新编译 | 无 | 用了 `file(GLOB)` | 显式列文件 |
| Debug 下崩溃、Release 正常 | 无 | UB（优化掩盖了问题） | 用 Debug + 断言排查 |
| 链接报重复定义 | `LNK2005` | 头文件里定义非 inline 函数 | 见 [[22 现代 .h 与模块：头文件的正确写法]] |
| CMake 找不到编译器 | `No CMAKE_CXX_COMPILER could be found` | 没跑 VS 开发者命令行 | 在 VS 命令提示符里执行 |

## 练习

1. 为一个"库 + 可执行文件 + 测试"的三目标项目写出 `CMakeLists.txt`。
2. 用生成器表达式给 MSVC 和 GCC 分别设置不同警告选项。
3. 解释 `PRIVATE` / `PUBLIC` / `INTERFACE` 的区别，各举一个真实例子。
4. 写一个 `CMakePresets.json`，定义 `debug` 与 `release` 两个预设。
5. 用 `FetchContent` 引入一个第三方库，并说明为什么比手工下载好。
6. 把本教程的 `proj_demo`（模块工程）改写成 CMake 版本（只需写出配置，说明哪些点需要 3.28+）。

> [!success]- 参考答案
> 1.
> ```cmake
> cmake_minimum_required(VERSION 3.28)
> project(Demo LANGUAGES CXX)
> add_library(core STATIC src/core.cpp)
> target_include_directories(core PUBLIC include)
> add_executable(app src/main.cpp)
> target_link_libraries(app PRIVATE core)
> enable_testing()
> add_executable(tests tests/test_core.cpp)
> target_link_libraries(tests PRIVATE core)
> add_test(NAME core_tests COMMAND tests)
> ```
> 2.
> ```cmake
> target_compile_options(app PRIVATE
>     $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
>     $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
> )
> ```
> 3. `PRIVATE`：只有自己编译时需要（如内部实现头文件目录）；`PUBLIC`：自己和链接者都需要（如库的公开头文件目录）；`INTERFACE`：自己编译不需要，但使用者需要（纯头文件库）。
> 4.
> ```json
> { "version": 6,
>   "configurePresets": [
>     { "name": "debug", "binaryDir": "build/debug", "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" } },
>     { "name": "release", "binaryDir": "build/release", "cacheVariables": { "CMAKE_BUILD_TYPE": "Release" } }
>   ] }
> ```
> 5. `FetchContent` 把依赖的下载、配置、构建纳入同一套流程，版本锁定在 `GIT_TAG`，CI 可复现；手工下载容易出现"我这里能跑你那里不行"。
> 6. 关键是 `target_sources(core PUBLIC FILE_SET CXX_MODULES FILES src/geo.ixx)`，并且 `cmake_minimum_required` ≥ 3.28、生成器用 Ninja 1.11+ 或 VS。

---

上一节：[[36 分配器与 pmr]] ｜ 下一节：[[38 测试、调试与 sanitizer]]
