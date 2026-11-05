import std;
int main(){ std::latch l(1); l.count_down(); l.wait(); std::barrier b(1); b.arrive_and_wait(); std::counting_semaphore<> s(1); s.release(); s.acquire(); }
