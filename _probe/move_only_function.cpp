import std;
int main(){ std::move_only_function<int(int)> f=[](int x){return x+1;}; std::println("{}", f(1)); }
