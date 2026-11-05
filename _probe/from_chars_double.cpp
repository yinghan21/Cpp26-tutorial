import std;
int main(){ double d{}; std::string s="3.5"; auto r = std::from_chars(s.data(), s.data()+s.size(), d); std::println("{} {}", d, (int)r.ec); }
