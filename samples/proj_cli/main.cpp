import std;

namespace fs = std::filesystem;

struct Options {
    std::string command = "help";
    std::string path = ".";
    bool verbose = false;
    bool follow_links = false;
};

void print_help() {
    std::println("用法: proj_cli <命令> [选项] [路径]");
    std::println("");
    std::println("命令:");
    std::println("  count   统计文件数量");
    std::println("  stat    统计文件数量与总大小");
    std::println("  help    显示帮助");
    std::println("");
    std::println("选项:");
    std::println("  -v, --verbose   输出每个文件");
    std::println("  -h, --help      显示帮助");
}

std::expected<Options, std::string> parse_args(std::span<char*> args) {
    Options opt;
    bool has_command = false;
    for (std::size_t i = 0; i < args.size(); ++i) {
        std::string_view a = args[i];
        if (a == "-h" || a == "--help") { opt.command = "help"; return opt; }
        if (a == "-v" || a == "--verbose") { opt.verbose = true; continue; }
        if (!a.empty() && a[0] == '-') return std::unexpected(std::format("未知选项: {}", a));
        if (!has_command) { opt.command = std::string{a}; has_command = true; }
        else opt.path = std::string{a};
    }
    if (!has_command) opt.command = "help";
    return opt;
}

int main(int argc, char** argv) {
    std::vector<char*> args(argv + 1, argv + argc);
    auto parsed = parse_args(args);
    if (!parsed) {
        std::println(std::cerr, "错误: {}", parsed.error());
        return 2;                                   // 用法错误
    }
    const auto& opt = *parsed;

    if (opt.command == "help") { print_help(); return 0; }

    std::error_code ec;
    if (!fs::exists(opt.path, ec)) {
        std::println(std::cerr, "错误: 路径不存在 {}", opt.path);
        return 1;
    }

    std::uintmax_t total_size = 0;
    std::size_t file_count = 0, dir_count = 0;
    for (const auto& e : fs::recursive_directory_iterator(opt.path, ec)) {
        if (ec) break;
        if (e.is_regular_file(ec)) {
            ++file_count;
            total_size += e.file_size(ec);
            if (opt.verbose) std::println("  {} ({} bytes)", e.path().string(), e.file_size(ec));
        } else if (e.is_directory(ec)) {
            ++dir_count;
        }
    }

    if (opt.command == "count") {
        std::println("{:<12}{:>8}", "文件数", file_count);
    } else if (opt.command == "stat") {
        std::println("{:<12}{:>8}", "文件数", file_count);
        std::println("{:<12}{:>8}", "目录数", dir_count);
        std::println("{:<12}{:>10.2f} KB", "总大小", total_size / 1024.0);
    } else {
        std::println(std::cerr, "错误: 未知命令 {}", opt.command);
        return 2;
    }
    return 0;
}
