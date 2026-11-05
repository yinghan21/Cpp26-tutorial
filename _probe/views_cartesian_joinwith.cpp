import std;
int main(){ std::vector<std::vector<int>> vv{{1},{2}}; for (int x : vv | std::views::join_with(0)) std::print("{}", x); for (auto t : std::views::cartesian_product(vv)) std::print("{}", std::get<0>(t).size()); }
