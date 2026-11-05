import std;
int main(){ auto f = std::async(std::launch::async, []{ return 7; }); std::println("{}", f.get()); }
