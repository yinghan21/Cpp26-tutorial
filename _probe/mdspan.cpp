import std;
int main(){ static int a[6]{}; std::mdspan m(a,2,3); std::println("{}", m.extent(0)); }
