# 探测本机 MSVC 19.51 对现代/C++23/26 库设施的可用性
$ErrorActionPreference = "Stop"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -property installationPath
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
$build = Join-Path (Split-Path $PSScriptRoot -Parent) "build"
$dir = $PSScriptRoot

$probes = @(
    @{ n = "println_chinese"; c = 'import std; int main(){ std::println("中文 {}", 1); }' },
    @{ n = "expected"; c = 'import std; int main(){ std::expected<int,std::string> e=1; std::println("{}", e.value()); }' },
    @{ n = "expected_monadic"; c = 'import std; int main(){ std::expected<int,std::string> e=1; auto r=e.and_then([](int x){return std::expected<int,std::string>(x+1);}); std::println("{}",r.value()); }' },
    @{ n = "generator"; c = 'import std; std::generator<int> g(){ co_yield 1; co_yield 2; } int main(){ int s=0; for(int x: g()) s+=x; std::println("{}",s); }' },
    @{ n = "flat_map"; c = 'import std; int main(){ std::flat_map<int,int> m; m[1]=2; std::println("{}", m[1]); }' },
    @{ n = "mdspan"; c = 'import std; int main(){ static int a[6]{}; std::mdspan m(a,2,3); std::println("{}", m.extent(0)); }' },
    @{ n = "stacktrace"; c = 'import std; int main(){ auto t = std::stacktrace::current(); std::println("{}", t.size()); }' },
    @{ n = "jthread"; c = 'import std; int main(){ std::jthread t([](std::stop_token s){ while(!s.stop_requested()) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }); }' },
    @{ n = "latch_barrier_sem"; c = 'import std; int main(){ std::latch l(1); l.count_down(); l.wait(); std::barrier b(1); b.arrive_and_wait(); std::counting_semaphore<> s(1); s.release(); s.acquire(); }' },
    @{ n = "spanstream"; c = 'import std; int main(){ char buf[32]{}; std::spanstream ss(std::span<char>(buf)); ss << 42; std::println("{}", ss.str().size()); }' },
    @{ n = "move_only_function"; c = 'import std; int main(){ std::move_only_function<int(int)> f=[](int x){return x+1;}; std::println("{}", f(1)); }' },
    @{ n = "rcu"; c = 'import std; int main(){ std::rcu_default_domain(); }' },
    @{ n = "simd"; c = 'import std; int main(){ std::simd<int> v(1); std::println("{}", v.size()); }' },
    @{ n = "inplace_vector"; c = 'import std; int main(){ std::inplace_vector<int,4> v; v.push_back(1); std::println("{}", v.size()); }' },
    @{ n = "hive"; c = 'import std; int main(){ std::hive<int> h; h.insert(1); std::println("{}", h.size()); }' },
    @{ n = "views_enumerate"; c = 'import std; int main(){ std::vector<int> v{1,2}; for (auto [i,x] : v | std::views::enumerate) std::println("{}{}", i, x); }' },
    @{ n = "views_chunk_zip"; c = 'import std; int main(){ std::vector<int> v{1,2,3,4}; for (auto c : v | std::views::chunk(2)) for (int x : c) std::print("{}", x); for (auto [a,b] : std::views::zip(v,v)) std::print("{}{}", a, b); }' },
    @{ n = "views_cartesian_joinwith"; c = 'import std; int main(){ std::vector<std::vector<int>> vv{{1},{2}}; for (int x : vv | std::views::join_with(0)) std::print("{}", x); for (auto t : std::views::cartesian_product(vv)) std::print("{}", std::get<0>(t).size()); }' },
    @{ n = "ranges_to"; c = 'import std; int main(){ std::vector<int> v{1,2,3}; auto s = v | std::views::transform([](int x){return x*2;}) | std::ranges::to<std::vector>(); std::println("{}", s.size()); }' },
    @{ n = "format_specs"; c = 'import std; int main(){ std::println("{:>8.3f}|{:x}|{:b}|{:08}", 3.14159, 255, 5, 42); }' },
    @{ n = "string_contains"; c = 'import std; int main(){ std::string s="hello"; std::println("{}", s.contains("ell")); }' },
    @{ n = "to_underlying"; c = 'import std; enum class E{a}; int main(){ std::println("{}", std::to_underlying(E::a)); }' },
    @{ n = "byteswap_bitcast"; c = 'import std; int main(){ std::println("{}", std::byteswap(0x1234u)); float f = std::bit_cast<float>(0x3f800000u); std::println("{}", f); }' },
    @{ n = "version"; c = 'import std; int main(){ std::println("{}", __cplusplus); }' },
    @{ n = "contracts"; c = 'int f(int x) [[pre: x > 0]] { return x; } int main(){ return f(1) - 1; }' },
    @{ n = "pack_indexing"; c = 'template<class... T> constexpr auto first(T... t){ return t...[0]; } static_assert(first(1,2,3)==1); int main(){}' },
    @{ n = "delete_reason"; c = 'struct A{ void f() = delete("gone"); }; int main(){ A a; a.f(); }' },
    @{ n = "placeholder_underscore"; c = 'int main(){ auto _ = 1; auto _ = 2; return 0; }' },
    @{ n = "reflection"; c = 'int main(){ constexpr auto r = ^^int; }' },
    @{ n = "filesystem"; c = 'import std; int main(){ std::filesystem::path p = "x"; std::println("{}", p.filename().string()); }' },
    @{ n = "constexpr_vector"; c = 'import std; constexpr std::size_t f(){ std::vector<int> v{1,2}; return v.size(); } static_assert(f()==2); int main(){}' },
    @{ n = "execution_sender"; c = 'import std; int main(){ auto s = std::execution::just(1); (void)s; }' },
    @{ n = "scope_exit"; c = 'import std; int main(){ auto g = std::scope_exit([]{ }); }' },
    @{ n = "atomic_ref"; c = 'import std; int main(){ int x = 0; std::atomic_ref<int> r(x); r.fetch_add(1); std::println("{}", x); }' },
    @{ n = "coroutine_task_custom"; c = 'import std; struct task { struct promise_type { task get_return_object(){return{};} std::suspend_never initial_suspend(){return{};} std::suspend_never final_suspend() noexcept {return{};} void return_void(){} void unhandled_exception(){} }; }; task f(){ co_return; } int main(){ f(); }' },
    @{ n = "print_bool_ptr"; c = 'import std; int main(){ int* p=nullptr; std::println("{} {}", true, p); }' }
)

$results = @()
foreach ($p in $probes) {
    $f = Join-Path $dir "$($p.n).cpp"
    # MSVC 要求 import 声明独占一行，探针源码因此把首个 import 换成换行
    $code = $p.c -replace '^import std; ', "import std;`n"
    [System.IO.File]::WriteAllText($f, $code + "`n", [System.Text.UTF8Encoding]::new($false))
    $obj = Join-Path $dir "$($p.n).obj"
    $log = cmd /c "`"$vcvars`" >nul 2>&1 && cl /nologo /std:c++latest /EHsc /utf-8 /wd5050 /c `"$f`" /ifcSearchDir `"$build`" /Fo:`"$obj`" 2>&1"
    $ok = ($log -notmatch "error ") -and (Test-Path $obj)
    $firstErr = ($log | Where-Object { $_ -match "error " } | Select-Object -First 1)
    $results += [pscustomobject]@{ Feature = $p.n; Support = if ($ok) { "OK" } else { "FAIL" }; Error = if ($firstErr) { ($firstErr -replace '.*error ', '').Trim() } else { "" } }
}

$results | Format-Table -AutoSize | Out-String -Width 200
