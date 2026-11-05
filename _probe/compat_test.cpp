import std.compat;

int main() {
    std::printf("%d %zu\n", 42, ::strlen("abc"));
    std::println("println also works");
}
