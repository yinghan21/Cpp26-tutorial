import std;
constexpr std::size_t f(){ std::vector<int> v{1,2}; return v.size(); } static_assert(f()==2); int main(){}
