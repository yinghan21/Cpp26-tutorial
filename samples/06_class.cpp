import std;

class Player {
public:
    Player(std::string name, int hp) : name_(std::move(name)), hp_(hp) {
        std::println("[构造] {}", name_);
    }
    ~Player() { std::println("[析构] {}", name_); }

    void damage(int d) { hp_ -= d; }
    bool alive() const { return hp_ > 0; }
    const std::string& name() const { return name_; }
    int hp() const { return hp_; }

private:
    std::string name_;
    int hp_;
};

int main() {
    {
        Player p{"一号", 100};
        p.damage(30);
        std::println("{} hp={} alive={}", p.name(), p.hp(), p.alive());
    }   // 这里自动析构

    auto* heap = new Player{"堆上的", 50};   // 不推荐，仅演示
    delete heap;                            // 必须手动释放 → 所以用智能指针
}