import std;

namespace fs = std::filesystem;

int main() {
    // ---- 路径操作 ----
    fs::path p = "dir/sub/file.txt";
    std::println("filename={} stem={} ext={} parent={}", p.filename().string(), p.stem().string(), p.extension().string(), p.parent_path().string());
    std::println("拼接: {}", (fs::path("a") / "b" / "c.txt").string());

    // ---- 临时目录与临时文件 ----
    fs::path tmpdir = fs::temp_directory_path();
    fs::path work = tmpdir / "cpp26_files_demo";
    fs::create_directories(work);

    // ---- 写文件 ----
    {
        std::ofstream out(work / "demo.txt");
        out << "line1\nline2\nline3\n";
    }   // RAII 自动关闭

    // ---- 读文件 ----
    {
        std::ifstream in(work / "demo.txt");
        std::string line;
        int count = 0;
        while (std::getline(in, line)) {
            ++count;
            std::println("读到[{}]: {}", count, line);
        }
    }

    // ---- 文件信息 ----
    std::println("exists={} size={}", fs::exists(work / "demo.txt"), fs::file_size(work / "demo.txt"));

    // ---- 遍历目录 ----
    for (const auto& e : fs::directory_iterator(work)) {
        std::println("  {} ({} bytes)", e.path().filename().string(), fs::is_regular_file(e) ? fs::file_size(e) : 0);
    }

    // ---- error_code 版本：不抛异常 ----
    std::error_code ec;
    auto sz = fs::file_size(work / "missing.txt", ec);
    std::println("读不存在文件: size={} ec={} msg={}", sz, ec.value(), ec.message());

    // ---- 抛异常版本 ----
    try {
        (void)fs::file_size(work / "missing.txt");
    } catch (const fs::filesystem_error& e) {
        std::println("filesystem_error: {}", e.what());
    }

    // ---- 清理 ----
    fs::remove_all(work);
    std::println("清理完成, 目录还在? {}", fs::exists(work));
}
