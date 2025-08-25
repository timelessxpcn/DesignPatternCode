#include <cassert>
#include <string>
#include <iostream>

// Minimal inline test using simplified strategy snippet
struct Strategy {
    virtual ~Strategy() = default;
    virtual int apply(int v) = 0;
};
struct Inc : Strategy { int apply(int v) override { return v+1; } };
struct DoubleS : Strategy { int apply(int v) override { return v*2; } };

struct Ctx {
    Strategy* s;
    explicit Ctx(Strategy* st): s(st) {}
    int run(int v){ return s->apply(v); }
};

int main(){
    Inc inc;
    DoubleS dbl;
    Ctx c1(&inc);
    assert(c1.run(5)==6);
    c1.s=&dbl;
    assert(c1.run(5)==10);
    std::cout << "strategy test ok\n";
}
