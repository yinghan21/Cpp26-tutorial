export module expr.eval;

import std;
import expr.parser;

export namespace expr {

export class Interpreter {
public:
    std::expected<double, std::string> eval(const NodePtr& node) {
        switch (node->kind) {
            case Node::Kind::Number:
                return node->value;
            case Node::Kind::Variable: {
                auto it = vars_.find(node->name);
                if (it == vars_.end()) return std::unexpected(std::format("未定义的变量: {}", node->name));
                return it->second;
            }
            case Node::Kind::Negate: {
                auto v = eval(node->lhs);
                if (!v) return std::unexpected(v.error());
                return -*v;
            }
            case Node::Kind::Binary: {
                auto l = eval(node->lhs);
                if (!l) return std::unexpected(l.error());
                auto r = eval(node->rhs);
                if (!r) return std::unexpected(r.error());
                switch (node->op) {
                    case '+': return *l + *r;
                    case '-': return *l - *r;
                    case '*': return *l * *r;
                    case '/':
                        if (*r == 0) return std::unexpected(std::string("除以零"));
                        return *l / *r;
                    case '%':
                        if (*r == 0) return std::unexpected(std::string("对零取模"));
                        return std::fmod(*l, *r);
                    default: return std::unexpected(std::string("未知运算符"));
                }
            }
            case Node::Kind::Assign:
                return std::unexpected(std::string("内部错误：赋值节点不应走到这里"));
        }
        return std::unexpected(std::string("未知节点"));
    }

    void set(std::string name, double value) { vars_[std::move(name)] = value; }

    const std::map<std::string, double>& vars() const { return vars_; }

private:
    std::map<std::string, double> vars_;
};

}
