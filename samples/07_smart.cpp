import std;

class Buffer {
public:
    explicit Buffer(std::size_t n) : data_(n) { std::println("Buffer({}) 创建", n); }
    ~Buffer() { std::println("Buffer 销毁"); }
    int& at(std::size_t i) { return data_[i]; }
    std::size_t size() const { return data_.size(); }
private:
    std::vector<int> data_;     // 用 vector，不手写 new/delete
};

int main() {
    auto p = std::make_unique<Buffer>(4);   // 独占所有权
    p->at(0) = 42;
    std::println("p[0]={} size={}", p->at(0), p->size());

    auto q = std::move(p);                  // 转移所有权
    std::println("p is null? {}", p == nullptr);
    std::println("q[0]={}", q->at(0));

    auto shared = std::make_shared<Buffer>(2);   // 共享所有权
    auto copy = shared;                          // 引用计数 +1
    std::println("use_count={}", shared.use_count());
}