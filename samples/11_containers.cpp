import std;

int main() {
    // ---- vector 基本操作 ----
    std::vector<int> v{5, 3, 9};
    v.push_back(1);
    v.reserve(8);
    std::println("size={} capacity={} front={} back={}", v.size(), v.capacity(), v.front(), v.back());
    v.insert(v.begin() + 1, 7);
    v.erase(v.begin());
    std::println("{}", v);

    // ---- reserve 的效果 ----
    std::vector<int> a, b;
    for (int i = 0; i < 100; ++i) a.push_back(i);
    b.reserve(100);
    for (int i = 0; i < 100; ++i) b.push_back(i);
    std::println("无 reserve capacity={} 有 reserve capacity={}", a.capacity(), b.capacity());

    // ---- emplace vs push ----
    std::vector<std::string> names;
    names.push_back(std::string(10, 'x'));     // 构造临时对象再移动
    names.emplace_back(10, 'y');               // 原地构造
    std::println("{} {}", names[0].size(), names[1].size());

    // ---- map / unordered_map ----
    std::map<std::string, int> ordered{{"b", 2}, {"a", 1}};
    std::unordered_map<std::string, int> hashed{{"b", 2}, {"a", 1}};
    ordered["c"] = 3;
    hashed["c"] = 3;
    std::print("map: ");
    for (const auto& [k, val] : ordered) std::print("{}={} ", k, val);
    std::print("| unordered (顺序不定): ");
    for (const auto& [k, val] : hashed) std::print("{}={} ", k, val);
    std::println("");

    // ---- contains / at / operator[] ----
    std::println("contains a={} at(a)={}", ordered.contains("a"), ordered.at("a"));
    std::println("不存在的 key 用 [] 会插入: size {} -> ", ordered.size());
    int zero = ordered["zzz"];                 // 插入 0
    std::println("size={} value={}", ordered.size(), zero);

    // ---- erase-remove ----
    std::vector<int> nums{1, 2, 3, 4, 5, 6};
    std::erase_if(nums, [](int x) { return x % 2 == 0; });
    std::println("删除偶数后: {}", nums);

    // ---- set / deque / list ----
    std::set<int> st{3, 1, 2};
    std::deque<int> dq{1, 2};
    std::list<int> lst{1, 2};
    st.insert(4); dq.push_front(0); lst.push_back(3);
    std::println("set={} deque={} list={}", st, dq, lst);
}
