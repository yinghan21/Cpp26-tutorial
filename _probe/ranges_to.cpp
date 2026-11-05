import std;
int main(){ std::vector<int> v{1,2,3}; auto s = v | std::views::transform([](int x){return x*2;}) | std::ranges::to<std::vector>(); std::println("{}", s.size()); }
