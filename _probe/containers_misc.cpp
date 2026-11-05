import std;
int main(){ std::set<int> s{3,1}; std::deque<int> d{1}; std::list<int> l{1}; std::stack<int> st; st.push(1); std::queue<int> q; q.push(2); std::priority_queue<int> pq; pq.push(3); std::bitset<8> b(5); std::println("{} {} {}", s.size(), b.to_string(), std::numbers::pi); }
