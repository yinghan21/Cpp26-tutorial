import std;

struct Record {
    int id;
    std::string name;
    double score;

    friend std::ostream& operator<<(std::ostream& os, const Record& r) {
        return os << r.id << ',' << r.name << ',' << r.score;
    }
    friend std::istream& operator>>(std::istream& is, Record& r) {
        std::string field;
        if (std::getline(is, field, ',')) r.id = std::stoi(field);
        if (std::getline(is, r.name, ',')) {}
        if (std::getline(is, field)) r.score = std::stod(field);
        return is;
    }
};

int main() {
    // ---- stringstream：字符串当流用 ----
    std::stringstream ss;
    ss << 42 << ' ' << 3.14 << ' ' << "text";
    int i; double d; std::string w;
    ss >> i >> d >> w;
    std::println("解析出 i={} d={} w={}", i, d, w);

    // ---- 状态位 ----
    std::istringstream bad("abc");
    int x{};
    bad >> x;
    std::println("fail={} eof={}", bad.fail(), bad.eof());

    // ---- 自定义 operator<< / >> ----
    Record r{1, "YG", 99.5};
    std::stringstream rec_ss;
    rec_ss << r;
    std::println("序列化: {}", rec_ss.str());
    Record back{};
    rec_ss >> back;
    std::println("反序列化: id={} name={} score={}", back.id, back.name, back.score);

    // ---- osyncstream：多线程安全输出 ----
    auto job = [](int id) {
        for (int k = 0; k < 2; ++k) {
            std::osyncstream(std::cout) << "线程 " << id << " 第 " << k << " 行\n";
        }
    };
    std::jthread t1{job, 1};
    std::jthread t2{job, 2};

    // ---- 简单序列化：CSV 汇总 ----
    std::vector<Record> records{{1, "a", 1.5}, {2, "b", 2.5}};
    std::stringstream csv;
    for (const auto& rec : records) csv << rec << '\n';
    std::println("CSV:\n{}", csv.str());

    // ---- 用 format 出报表 ----
    std::println("{:<4}{:<6}{:>8}", "id", "name", "score");
    for (const auto& rec : records) std::println("{:<4}{:<6}{:>8.2f}", rec.id, rec.name, rec.score);

    t1.join();
    t2.join();
}
