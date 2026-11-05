import std;
std::generator<int> g(){ co_yield 1; co_yield 2; } int main(){ int s=0; for(int x: g()) s+=x; std::println("{}",s); }
