import std;
int main(){ std::mutex m; std::condition_variable cv; std::unique_lock lk(m); std::atomic<int> a{0}; a.fetch_add(1); std::println("{}", a.load()); }
