struct A { void g() = delete; };
int main() { A a; a.g(); }
