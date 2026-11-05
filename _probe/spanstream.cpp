import std;
int main(){ char buf[32]{}; std::spanstream ss{std::span<char>(buf)}; ss << 42; std::println("{}", ss.str().size()); }
