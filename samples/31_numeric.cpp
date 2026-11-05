import std;

int main() {
    // ---- std::numbers ----
    std::println("pi={:.6f} e={:.6f} sqrt2={:.6f}", std::numbers::pi, std::numbers::e, std::numbers::sqrt2);

    // ---- <bit> ----
    std::println("popcount(0b1011)={}", std::popcount(0b1011u));
    std::println("countl_zero(1u)={} countr_zero(8u)={}", std::countl_zero(1u), std::countr_zero(8u));
    std::println("bit_width(255)={} bit_floor(100)={} bit_ceil(100)={}", std::bit_width(255u), std::bit_floor(100u), std::bit_ceil(100u));
    std::println("has_single_bit(64)={}", std::has_single_bit(64u));
    std::println("rotl(0b0001,2)={:04b}", std::rotl(0b0001u, 2));
    std::println("byteswap(0x1234)={:#x}", std::byteswap(std::uint16_t{0x1234}));

    float f = std::bit_cast<float>(0x3f800000u);
    std::println("bit_cast -> {}", f);

    // ---- <numeric> ----
    std::vector<int> v{1, 2, 3, 4, 5};
    std::println("accumulate={}", std::accumulate(v.begin(), v.end(), 0));
    std::println("reduce={}", std::reduce(v.begin(), v.end(), 0));
    std::println("gcd(48,18)={} lcm(4,6)={}", std::gcd(48, 18), std::lcm(4, 6));
    std::println("midpoint(10,20)={}", std::midpoint(10, 20));

    std::vector<int> seq(5);
    std::iota(seq.begin(), seq.end(), 10);
    std::println("iota -> {}", seq);

    // ---- 位标志 ----
    enum class Perm : std::uint8_t { read = 1, write = 2, exec = 4 };
    auto has = [](Perm mask, Perm bit) { return (std::to_underlying(mask) & std::to_underlying(bit)) != 0; };
    Perm p = static_cast<Perm>(std::to_underlying(Perm::read) | std::to_underlying(Perm::write));
    std::println("read={} write={} exec={}", has(p, Perm::read), has(p, Perm::write), has(p, Perm::exec));

    // ---- 溢出与比较 ----
    std::println("cmp_less(-1, 1u)={}", std::cmp_less(-1, 1u));
    std::println("int max={}", std::numeric_limits<int>::max());

    // ---- 浮点特殊值 ----
    double inf = std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();
    std::println("isfinite(inf)={} isnan(nan)={}", std::isfinite(inf), std::isnan(nan));
}
