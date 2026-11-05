import std;

// 自定义分配器：统计分配次数与总字节
struct CountingResource : std::pmr::memory_resource {
    std::size_t allocations = 0;
    std::size_t bytes = 0;

    void* do_allocate(std::size_t n, std::size_t align) override {
        ++allocations;
        bytes += n;
        return std::pmr::new_delete_resource()->allocate(n, align);
    }
    void do_deallocate(void* p, std::size_t n, std::size_t align) override {
        std::pmr::new_delete_resource()->deallocate(p, n, align);
    }
    bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }
};

int main() {
    // ---- 默认资源 vs 单调缓冲资源 ----
    std::array<std::byte, 4096> buffer{};
    std::pmr::monotonic_buffer_resource pool{buffer.data(), buffer.size()};

    std::pmr::vector<int> v{&pool};
    for (int i = 0; i < 100; ++i) v.push_back(i);
    std::println("monotonic 容量={} 大小={}", v.capacity(), v.size());

    // ---- 统计分配 ----
    CountingResource counter;
    {
        std::pmr::vector<std::string> names{&counter};
        names.emplace_back("alpha");
        names.emplace_back("beta");
        std::println("names={}", names.size());
    }
    std::println("分配次数={} 总字节={}", counter.allocations, counter.bytes);

    // ---- 分配器传播：拷贝时不带资源 ----
    std::pmr::vector<int> a{&pool};
    a.push_back(1);
    std::pmr::vector<int> b = a;          // 拷贝构造：继承资源
    std::println("拷贝后 b 使用同一资源? {}", b.get_allocator().resource() == a.get_allocator().resource());

    // ---- 对象池思路：单调资源一次释放 ----
    {
        std::array<std::byte, 1024> buf{};
        std::pmr::monotonic_buffer_resource res{buf.data(), buf.size()};
        std::pmr::vector<std::pmr::string> nested{&res};
        for (int i = 0; i < 10; ++i) nested.emplace_back("item" + std::to_string(i));
        std::println("嵌套容器第一项={}", nested.front());
        // 离开作用域：res 一次性释放所有内存，无需逐个 delete
    }
}
