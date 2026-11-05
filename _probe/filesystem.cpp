import std;
int main(){ std::filesystem::path p = "x"; std::println("{}", p.filename().string()); }
