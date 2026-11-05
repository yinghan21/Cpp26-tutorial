import std;
int main(){ std::jthread t([](std::stop_token s){ while(!s.stop_requested()) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }); }
