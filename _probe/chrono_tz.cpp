import std;
int main(){ auto z = std::chrono::current_zone(); std::println("{}", z->name()); }
