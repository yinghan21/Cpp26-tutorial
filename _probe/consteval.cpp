import std;
consteval int sq(int x){ return x*x; } static_assert(sq(4)==16); int main(){ std::println("{}", sq(5)); }
