import std;
int main(){ std::optional<int> o = 4; auto r = o.transform([](int x){return x*2;}).value_or(-1); std::println("{}", r); }
