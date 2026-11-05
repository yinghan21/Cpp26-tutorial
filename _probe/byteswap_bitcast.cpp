import std;
int main(){ std::println("{}", std::byteswap(0x1234u)); float f = std::bit_cast<float>(0x3f800000u); std::println("{}", f); }
