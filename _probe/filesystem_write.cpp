import std;
int main(){ std::filesystem::path p = std::filesystem::temp_directory_path() / "probe.txt"; { std::ofstream f(p); f << "x"; } std::println("{}", std::filesystem::file_size(p)); std::filesystem::remove(p); }
