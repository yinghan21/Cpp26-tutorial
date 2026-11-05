import std;
int main(){ std::regex re("\\d+"); std::smatch m; std::string s="a12b"; std::regex_search(s,m,re); std::println("{}", m.str()); }
