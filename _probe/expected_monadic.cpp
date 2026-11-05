import std;
int main(){ std::expected<int,std::string> e=1; auto r=e.and_then([](int x){return std::expected<int,std::string>(x+1);}); std::println("{}",r.value()); }
