import std;

struct Point { char c; int i; double d; };
struct Packed { double d; int i; char c; };

struct alignas(64) CacheLine { int v; };

int main() {
    // ---- 布局与 padding ----
    std::println("sizeof(Point)={} sizeof(Packed)={}", sizeof(Point), sizeof(Packed));
    Point po{};
    Packed pk{};
    auto off = [](const void* base, const void* member) {
        return static_cast<std::ptrdiff_t>(static_cast<const char*>(member) - static_cast<const char*>(base));
    };
    std::println("Point 偏移: c={} i={} d={}", off(&po, &po.c), off(&po, &po.i), off(&po, &po.d));
    std::println("Packed 偏移: d={} i={} c={}", off(&pk, &pk.d), off(&pk, &pk.i), off(&pk, &pk.c));

    // ---- 对齐 ----
    std::println("alignof(int)={} alignof(double)={} alignof(CacheLine)={}",
                 alignof(int), alignof(double), alignof(CacheLine));
    std::println("sizeof(CacheLine)={}", sizeof(CacheLine));

    // ---- std::byte ----
    std::byte b{0x2A};
    std::println("std::byte -> {}", std::to_integer<int>(b));

    // ---- placement new：在指定内存上构造 ----
    alignas(std::string) std::byte buffer[sizeof(std::string)];
    auto* p = new (buffer) std::string("in place");
    std::println("placement new 得到: {}", *p);
    p->~basic_string();          // 必须显式析构

    // ---- mdspan：多维视图 ----
    std::vector<int> data(6);
    std::iota(data.begin(), data.end(), 1);
    std::mdspan m(data.data(), 2, 3);
    std::println("mdspan 维度: {} x {}", m.extent(0), m.extent(1));
    for (std::size_t r = 0; r < m.extent(0); ++r) {
        for (std::size_t c = 0; c < m.extent(1); ++c) std::print("{} ", m[r, c]);
        std::println("");
    }
    std::println("mdspan 元素总数={}", m.size());

    // ---- 伪共享演示 ----
    auto bench = [](bool padded) {
        struct Shared { std::atomic<int> a{0}; std::atomic<int> b{0}; };
        struct PaddedAtom { std::atomic<int> v{0}; char pad[64 - sizeof(std::atomic<int>)]; };
        struct Padded { PaddedAtom a, b; };
        (void)padded;
        Shared s; Padded pp;
        auto t0 = std::chrono::steady_clock::now();
        std::jthread t1([&] { for (int i = 0; i < 2000000; ++i) { s.a.fetch_add(1, std::memory_order_relaxed); pp.a.v.fetch_add(1, std::memory_order_relaxed); } });
        std::jthread t2([&] { for (int i = 0; i < 2000000; ++i) { s.b.fetch_add(1, std::memory_order_relaxed); pp.b.v.fetch_add(1, std::memory_order_relaxed); } });
        t1.join(); t2.join();
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    };
    std::println("伪共享 vs 对齐填充 耗时: {:.1f} ms", bench(true));
}
