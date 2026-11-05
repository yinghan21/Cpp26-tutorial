struct A{ void f() = delete("gone"); }; int main(){ A a; a.f(); }
