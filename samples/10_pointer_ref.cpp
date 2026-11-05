import std;

void add_by_value(int x) { x += 1; }
void add_by_ref(int& x) { x += 1; }
void add_by_ptr(int* x) { if (x) *x += 1; }

int main() {
    // ---- 传参的三种写法 ----
    int a = 1;
    add_by_value(a); std::println("after value: {}", a);
    add_by_ref(a);   std::println("after ref:   {}", a);
    add_by_ptr(&a);  std::println("after ptr:   {}", a);

    // ---- 指针算术 ----
    int arr[5]{10, 20, 30, 40, 50};
    int* p = arr;
    int* q = p + 3;
    std::println("*p={} *(p+3)={} q-p={} 差字节={}", *p, *(p + 3), q - p, reinterpret_cast<char*>(q) - reinterpret_cast<char*>(p));
    for (int* it = arr; it != arr + 5; ++it) std::print("{} ", *it);
    std::println("");

    // ---- nullptr ----
    int* np = nullptr;
    std::println("np == nullptr ? {}", np == nullptr);
    std::println("地址（转成整数打印）: {:#x}", reinterpret_cast<std::uintptr_t>(arr));

    // ---- 引用 vs 指针 ----
    int v = 5;
    int& rv = v;              // 引用必须初始化，且不能改绑
    rv = 6;
    std::println("v={} rv={} 同一对象={}", v, rv, &v == &rv);

    // ---- const 的四种位置 ----
    int m = 1;
    const int* p1 = &m;        // 指向 const int：不能通过 p1 改 m
    int* const p2 = &m;        // const 指针：不能指向别处，但能改 m
    const int* const p3 = &m;  // 两者都不能
    *p2 = 2;
    std::println("m={} p1->{}", m, *p1);
    (void)p1; (void)p3;

    // ---- 底层 const 传递：const 成员函数 ----
    struct S {
        int x = 1;
        int get() const { return x; }     // const 成员函数：不能改 x
    };
    const S s{};
    std::println("const S get={}", s.get());
}
