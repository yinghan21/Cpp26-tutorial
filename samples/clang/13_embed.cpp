// #embed（C++26）：把文件内容嵌进二进制
constexpr unsigned char blob[] = {
#embed "data.txt"
};

static_assert(sizeof(blob) == 3, "embed size mismatch");

int main() { return 0; }