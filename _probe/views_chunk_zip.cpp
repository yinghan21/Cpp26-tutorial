import std;
int main(){ std::vector<int> v{1,2,3,4}; for (auto c : v | std::views::chunk(2)) for (int x : c) std::print("{}", x); for (auto [a,b] : std::views::zip(v,v)) std::print("{}{}", a, b); }
