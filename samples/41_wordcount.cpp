import std;

// ---- 词频统计：从最小可用到优化版 ----
std::string make_sample_text() {
    return "the quick brown fox jumps over the lazy dog\n"
           "the dog barks and the fox runs\n"
           "a quick brown dog jumps\n";
}

// 1) 朴素版：全部拷贝
std::vector<std::pair<std::string, int>> word_count_naive(const std::string& text) {
    std::unordered_map<std::string, int> freq;
    std::string word;
    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c))) word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        else if (!word.empty()) { ++freq[word]; word.clear(); }
    }
    if (!word.empty()) ++freq[word];

    std::vector<std::pair<std::string, int>> out(freq.begin(), freq.end());
    std::ranges::sort(out, [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });
    return out;
}

// 2) 优化版：string_view 分词，零拷贝
std::vector<std::pair<std::string_view, int>> word_count_view(std::string_view text) {
    std::unordered_map<std::string_view, int> freq;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && !std::isalpha(static_cast<unsigned char>(text[i]))) ++i;
        std::size_t start = i;
        while (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i]))) ++i;
        if (i > start) ++freq[text.substr(start, i - start)];
    }
    std::vector<std::pair<std::string_view, int>> out(freq.begin(), freq.end());
    std::ranges::sort(out, [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });
    return out;
}

int main() {
    std::string text = make_sample_text();

    auto t0 = std::chrono::steady_clock::now();
    auto naive = word_count_naive(text);
    auto t1 = std::chrono::steady_clock::now();
    auto view = word_count_view(text);
    auto t2 = std::chrono::steady_clock::now();

    std::println("=== 朴素版（拷贝）===");
    for (const auto& [w, c] : naive | std::views::take(5)) std::println("{:<8} {}", w, c);

    std::println("=== 优化版（string_view）===");
    for (const auto& [w, c] : view | std::views::take(5)) std::println("{:<8} {}", w, c);

    std::println("耗时: 朴素 {:.4f} ms / 优化 {:.4f} ms",
                 std::chrono::duration<double, std::milli>(t1 - t0).count(),
                 std::chrono::duration<double, std::milli>(t2 - t1).count());

    // ---- 大规模对比 ----
    std::string big;
    big.reserve(1'000'000);
    for (int i = 0; i < 20000; ++i) big += text;
    auto t3 = std::chrono::steady_clock::now();
    auto big_naive = word_count_naive(big);
    auto t4 = std::chrono::steady_clock::now();
    auto big_view = word_count_view(big);
    auto t5 = std::chrono::steady_clock::now();
    std::println("大文本 {} 字节: 朴素 {:.1f} ms / 优化 {:.1f} ms (Top1={}:{})",
                 big.size(),
                 std::chrono::duration<double, std::milli>(t4 - t3).count(),
                 std::chrono::duration<double, std::milli>(t5 - t4).count(),
                 big_view[0].first, big_view[0].second);
}
