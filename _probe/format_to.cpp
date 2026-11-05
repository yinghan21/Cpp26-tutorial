import std;
int main(){ std::string out; std::format_to(std::back_inserter(out), "{}-{}", 1, 2); std::println("{}", out); }
