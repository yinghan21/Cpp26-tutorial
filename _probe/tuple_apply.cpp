import std;
int main(){ auto t = std::tuple{1, 2.5, "x"}; std::apply([](auto... a){ ((std::print("{} ", a)), ...); }, t); }
