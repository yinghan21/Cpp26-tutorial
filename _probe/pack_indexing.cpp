template<class... T> constexpr auto first(T... t){ return t...[0]; } static_assert(first(1,2,3)==1); int main(){}
