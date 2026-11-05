import std;
int main(){ auto now = std::chrono::system_clock::now(); std::println("{:%Y-%m-%d}", now); }
