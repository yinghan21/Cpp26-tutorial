import std;

std::vector<std::string_view> split_words(std::string_view text) {
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && !std::isalpha(static_cast<unsigned char>(text[i]))) ++i;
        std::size_t start = i;
        while (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i]))) ++i;
        if (i > start) out.push_back(text.substr(start, i - start));
    }
    return out;
}

std::string lower(std::string_view s) {
    std::string r{s};
    std::ranges::transform(r, r.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

int main() {
    const std::string text = "the quick brown fox jumps over the lazy dog the end";
    auto words = split_words(text);

    std::map<std::string, int> freq;
    for (auto w : words) ++freq[lower(w)];

    std::vector<std::pair<std::string, int>> ranked(freq.begin(), freq.end());
    std::ranges::sort(ranked, [](auto& a, auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });

    std::println("words={} unique={}", words.size(), ranked.size());
    for (std::size_t i = 0; i < ranked.size() && i < 3; ++i)
        std::println("{}. {} -> {}", i + 1, ranked[i].first, ranked[i].second);
}