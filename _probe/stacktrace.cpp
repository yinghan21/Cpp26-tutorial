import std;
int main(){ auto t = std::stacktrace::current(); std::println("{}", t.size()); }
