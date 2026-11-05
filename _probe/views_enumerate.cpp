import std;
int main(){ std::vector<int> v{1,2}; for (auto [i,x] : v | std::views::enumerate) std::println("{}{}", i, x); }
