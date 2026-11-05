import std;
int main(){ int x = 0; std::atomic_ref<int> r(x); r.fetch_add(1); std::println("{}", x); }
