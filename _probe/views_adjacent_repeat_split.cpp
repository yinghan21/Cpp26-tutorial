import std;
int main(){ std::vector<int> v{1,2,3}; for (auto t : v | std::views::adjacent<2>) std::print("{}", std::get<0>(t)); for (int x : std::views::repeat(7, 2)) std::print("{}", x); std::string s="a,b"; for (auto part : s | std::views::split(std::string{","})) std::print("{}", std::string_view(part).size()); }
