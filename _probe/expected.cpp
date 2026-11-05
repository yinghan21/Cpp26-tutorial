import std;
int main(){ std::expected<int,std::string> e=1; std::println("{}", e.value()); }
