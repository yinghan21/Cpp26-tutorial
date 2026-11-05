# 第二批探针：标准库覆盖面 + 模块机制
$ErrorActionPreference = "Stop"
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
$build = Join-Path (Split-Path $PSScriptRoot -Parent) "build"
$dir = $PSScriptRoot

$probes = @(
    @{ n = "spanstream"; c = 'import std; int main(){ char buf[32]{}; std::spanstream ss{std::span<char>(buf)}; ss << 42; std::println("{}", ss.str().size()); }' },
    @{ n = "regex"; c = 'import std; int main(){ std::regex re("\\d+"); std::smatch m; std::string s="a12b"; std::regex_search(s,m,re); std::println("{}", m.str()); }' },
    @{ n = "source_location"; c = 'import std; void f(std::source_location loc = std::source_location::current()){ std::println("{}:{}", loc.line(), loc.function_name()); } int main(){ f(); }' },
    @{ n = "chrono_tz"; c = 'import std; int main(){ auto z = std::chrono::current_zone(); std::println("{}", z->name()); }' },
    @{ n = "fold_left"; c = 'import std; int main(){ std::vector<int> v{1,2,3}; std::println("{}", std::ranges::fold_left(v, 0, std::plus<>())); }' },
    @{ n = "views_adjacent_repeat_split"; c = 'import std; int main(){ std::vector<int> v{1,2,3}; for (auto t : v | std::views::adjacent<2>) std::print("{}", std::get<0>(t)); for (int x : std::views::repeat(7, 2)) std::print("{}", x); std::string s="a,b"; for (auto part : s | std::views::split(std::string{","})) std::print("{}", std::string_view(part).size()); }' },
    @{ n = "range_format"; c = 'import std; int main(){ std::vector<int> v{1,2,3}; std::println("{}", v); }' },
    @{ n = "format_to"; c = 'import std; int main(){ std::string out; std::format_to(std::back_inserter(out), "{}-{}", 1, 2); std::println("{}", out); }' },
    @{ n = "print_cerr"; c = 'import std; int main(){ std::print(std::cerr, "err\n"); }' },
    @{ n = "from_chars_double"; c = 'import std; int main(){ double d{}; std::string s="3.5"; auto r = std::from_chars(s.data(), s.data()+s.size(), d); std::println("{} {}", d, (int)r.ec); }' },
    @{ n = "syncstream"; c = 'import std; int main(){ std::osyncstream os(std::cout); os << "a\n"; }' },
    @{ n = "print_string_view"; c = 'import std; int main(){ std::string_view sv="hi"; std::println("[{}] {}", sv, std::string{sv}); }' },
    @{ n = "tuple_apply"; c = 'import std; int main(){ auto t = std::tuple{1, 2.5, "x"}; std::apply([](auto... a){ ((std::print("{} ", a)), ...); }, t); }' },
    @{ n = "optional_monadic"; c = 'import std; int main(){ std::optional<int> o = 4; auto r = o.transform([](int x){return x*2;}).value_or(-1); std::println("{}", r); }' },
    @{ n = "format_colored"; c = 'import std; int main(){ std::println("{:*^9}|{:+#x}|{:.3e}", "ab", 255, 12345.678); }' },
    @{ n = "containers_misc"; c = 'import std; int main(){ std::set<int> s{3,1}; std::deque<int> d{1}; std::list<int> l{1}; std::stack<int> st; st.push(1); std::queue<int> q; q.push(2); std::priority_queue<int> pq; pq.push(3); std::bitset<8> b(5); std::println("{} {} {}", s.size(), b.to_string(), std::numbers::pi); }' },
    @{ n = "async_future"; c = 'import std; int main(){ auto f = std::async(std::launch::async, []{ return 7; }); std::println("{}", f.get()); }' },
    @{ n = "cv_mutex"; c = 'import std; int main(){ std::mutex m; std::condition_variable cv; std::unique_lock lk(m); std::atomic<int> a{0}; a.fetch_add(1); std::println("{}", a.load()); }' },
    @{ n = "std_compat"; c = 'import std.compat;
int main(){ std::printf("%d\n", 42); }' },
    @{ n = "local_time"; c = 'import std; int main(){ auto now = std::chrono::system_clock::now(); std::println("{:%Y-%m-%d}", now); }' },
    @{ n = "print_float_spec"; c = 'import std; int main(){ std::println("{:10.2f}|{:<5}|{:^7}", 1.5, 1, 2); }' },
    @{ n = "consteval"; c = 'import std; consteval int sq(int x){ return x*x; } static_assert(sq(4)==16); int main(){ std::println("{}", sq(5)); }' },
    @{ n = "string_view_substr"; c = 'import std; int main(){ std::string_view sv = "hello world"; std::println("{}", sv.substr(0, 5)); }' },
    @{ n = "filesystem_write"; c = 'import std; int main(){ std::filesystem::path p = std::filesystem::temp_directory_path() / "probe.txt"; { std::ofstream f(p); f << "x"; } std::println("{}", std::filesystem::file_size(p)); std::filesystem::remove(p); }' }
)

$results = @()
foreach ($p in $probes) {
    $f = Join-Path $dir "$($p.n).cpp"
    $code = $p.c -replace '^import std; ', "import std;`n" -replace '^import std.compat;', "import std.compat;"
    [System.IO.File]::WriteAllText($f, $code + "`n", [System.Text.UTF8Encoding]::new($false))
    $obj = Join-Path $dir "$($p.n).obj"
    $exe = Join-Path $dir "$($p.n).exe"
    $stdObj = Join-Path $build "std.obj"
    $log = cmd /c "`"$vcvars`" >nul 2>&1 && cl /nologo /std:c++latest /EHsc /utf-8 /wd5050 /c `"$f`" /ifcSearchDir `"$build`" /Fo:`"$obj`" 2>&1"
    $compiled = ($log -notmatch "error ") -and (Test-Path $obj)
    $run = ""
    if ($compiled) {
        $linkOut = cmd /c "`"$vcvars`" >nul 2>&1 && link /nologo `"$obj`" `"$stdObj`" /OUT:`"$exe`" 2>&1"
        if (Test-Path $exe) { $run = (cmd /c "`"$exe`" 2>&1" | Out-String).Trim() } else { $compiled = $false; $log = $linkOut }
    }
    $firstErr = ($log | Where-Object { $_ -match "error " } | Select-Object -First 1)
    $results += [pscustomobject]@{
        Feature = $p.n
        Compile = if ($compiled) { "OK" } else { "FAIL" }
        Run     = if ($run.Length -gt 60) { $run.Substring(0, 60) } else { $run }
        Error   = if ($firstErr) { ($firstErr -replace '.*error ', '').Trim() } else { "" }
    }
}
$results | Format-Table -AutoSize -Wrap | Out-String -Width 160
