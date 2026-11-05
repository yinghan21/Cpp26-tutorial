import std;
import expr.lexer;
import expr.parser;
import expr.eval;

namespace {

// 执行一行输入（支持 "x = expr" 与纯表达式）
std::optional<std::string> run_line(std::string_view line, expr::Interpreter& interp) {
    expr::Lexer lexer{line};
    auto tokens = lexer.tokenize();
    if (!tokens) return tokens.error();

    expr::Parser parser{*tokens};

    // 先尝试赋值语句
    auto assign = parser.parse_assignment();
    if (assign) {
        auto value = interp.eval(assign->root);
        if (!value) return value.error();
        interp.set(assign->assigned_name, *value);
        return std::nullopt;
    }

    // 再按普通表达式解析
    expr::Parser parser2{*tokens};
    auto node = parser2.parse_all();
    if (!node) return node.error();

    auto value = interp.eval(*node);
    if (!value) return value.error();
    return std::format("{}", *value);
}

}

int main(int argc, char** argv) {
    expr::Interpreter interp;
    interp.set("pi", 3.141592653589793);

    // 命令行参数模式：每行一个表达式
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string_view line{argv[i]};
            if (auto result = run_line(line, interp)) {
                std::println("> {} = {}", line, *result);
            } else {
                std::println("> {} (已赋值)", line);
            }
        }
        std::print("变量表: ");
        for (const auto& [k, v] : interp.vars()) std::print("{}={} ", k, v);
        std::println("");
        return 0;
    }

    // 交互模式（REPL）
    std::string line;
    std::println("表达式解释器（输入 q 退出）");
    while (true) {
        std::print("> ");
        if (!std::getline(std::cin, line)) break;
        if (line == "q" || line.empty()) break;
        if (auto result = run_line(line, interp)) std::println("  = {}", *result);
    }
    return 0;
}
