import std;
void f(std::source_location loc = std::source_location::current()){ std::println("{}:{}", loc.line(), loc.function_name()); } int main(){ f(); }
