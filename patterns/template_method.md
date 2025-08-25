# 模板方法模式 (Template Method Pattern)

## 1. 意图
在基类中定义算法骨架，将某些步骤延迟到子类实现或钩子函数，使结构稳定而步骤可定制。

## 2. 问题背景 / 动机
例如设备启动序列：`prepare() -> initHardware() -> loadConfig() -> startScheduler() -> postCheck()`。不同产品线只有个别步骤不同。若复制粘贴全套序列易失配。模板方法统一骨架，变化点抽象。

## 3. 适用场景
- 启动/关闭流程（部分步骤差异）
- 固件升级过程（校验 → 写入块 → 校验 → 激活）
- 数据处理固定壳 + 可变滤波步骤
- 测试序列固定结构 + 自定义钩子

## 4. 结构概述
基类 `run()` 定义步骤执行顺序，调用虚函数/钩子；子类覆盖。嵌入式可使用：
- 虚函数（动态差异）
- CRTP 静态多态内联
- 函数指针回调结构体

## 5. 基础实现（虚函数）
```cpp
class StartupSequence {
public:
    bool run(){
        if(!prepare()) return false;
        if(!initHardware()) return false;
        if(!loadConfig()) return false;
        startServices();
        postCheck();
        return true;
    }
protected:
    virtual bool prepare(){ return true; }
    virtual bool initHardware()=0;
    virtual bool loadConfig(){ return true; }
    virtual void startServices(){}
    virtual void postCheck(){}
};
class ProductAStartup : public StartupSequence {
protected:
    bool initHardware() override { /* A 初始化 */ return true; }
};
```

## 6. 嵌入式适配要点
- 避免多层继承：差异少用回调结构替代。
- 错误传播：返回 `bool` 或错误码枚举。
- 不在构造函数调用虚函数（初始化顺序风险）。
- 步骤可能耗时：可拆分成状态机避免阻塞（协程/Task）。

## 7. 进阶实现
### 7.1 CRTP 静态多态
```cpp
template<typename Derived>
class StartupCRTP {
public:
    bool run(){
        auto& d = static_cast<Derived&>(*this);
        if(!d.initHardware()) return false;
        d.loadConfig();
        return true;
    }
};
class ProductB : public StartupCRTP<ProductB> {
public:
    bool initHardware(){ /*...*/ return true; }
    void loadConfig(){ /*...*/ }
};
```
无虚表，步骤内联。

### 7.2 函数指针表
```cpp
struct StartupHooks {
    bool (*initHardware)();
    void (*loadConfig)();
};
bool run(const StartupHooks& h){
    if(!h.initHardware()) return false;
    h.loadConfig();
    return true;
}
```

### 7.3 分步非阻塞（状态机）
将每个步骤映射到状态，在循环中推进以保证实时性。

## 8. 资源与实时性评估
- 虚函数：每步骤一次间接
- CRTP：0 额外开销，但代码可能重复
- 状态机：更多状态逻辑，但无长阻塞

## 9. 替代比较
- 策略：同一算法点替换；模板方法定义顺序结构。
- 命令队列：动态可重排步骤；模板方法固定顺序。
- 构建器：构造复杂对象，不定义执行时序。

## 10. 可测试性与可维护性
- Mock 子类覆盖关键步骤统计调用顺序。
- CRTP 测试：实例化子类并运行。
- 步骤返回码注入故障测试回滚。

## 11. 适用性检查清单
Use：
- 算法整体稳定
- 仅少数步骤变化
Avoid：
- 步骤顺序频繁改变（改策略或配置驱动）
- 每个步骤逻辑都完全不同（策略组合更好）

## 12. 常见误用 & 修正
误用：基类过多保护成员供子类访问造成耦合。  
修正：传递参数或返回值，保持最小接口。

误用：在构造中执行 run()（未完全构造）。  
修正：外部显式调用。

## 13. 与其它模式组合
- 工厂：在 run 前创建资源
- 状态机：非阻塞拆分步骤
- 策略：某步骤内部使用策略选择算法

## 14. 案例
固件升级：预检 -> 擦除 -> 分块写 -> 校验 -> 激活；不同产品仅擦除/写差异。

## 15. 实验建议
- 比较虚函数 vs CRTP 执行时间（多次 run）
- Flash 尺寸对比

## 16. MISRA / AUTOSAR 注意
- 虚函数使用说明
- 返回码检查，禁止忽略

## 17. 面试 / Review 问题
- 与策略的边界？
- 如何避免子类滥用基类内部状态？
- CRTP 替代虚函数的利弊？

## 18. 练习
基础：实现 ProductC 含自定义 loadConfig。
提高：CRTP 版本 + 增加错误处理。
思考：将阻塞擦除步骤改成非阻塞状态机的设计。

## 19. 快速回顾
- 固定骨架 + 可变步骤
- 嵌入式可用 CRTP / 回调减少开销
- 注意非阻塞与错误传播
```

````markdown name=patterns/visitor.md
# 访问者模式 (Visitor Pattern)（精简 + variant 替代）

## 1. 意图
在不改变元素类的前提下为其增加新的操作，将操作与数据结构分离。

## 2. 问题背景 / 动机
嵌入式有时需要对一组不同类型节点（菜单项、消息不同类型、AST）执行多种操作（渲染/统计/序列化）。传统方式：在每个类里添加新方法增加代码耦合。访问者将操作独立。缺点：增加样板、对代码尺寸不友好，且类型集合需稳定。

## 3. 适用场景
- 稳定的节点类型集合，操作集合可能扩展
- 需要多种遍历操作：打印、编码、大小估算
- 资源允许一些样板（不太大）

若类型集合易变或少量类型：不要用访问者，直接 `switch` + `variant` 更适用。

## 4. 结构概述
Element 基类定义 `accept(Visitor&)`; Visitor 提供 `visit(Type&)` 重载；运行时双分派调用正确重载。替代：`std::variant` + `std::visit` 静态分派。

## 5. 基础实现（经典）
```cpp
struct MenuItem;
struct MenuGroup;

struct Visitor {
    virtual ~Visitor()=default;
    virtual void visit(MenuItem&)=0;
    virtual void visit(MenuGroup&)=0;
};

struct Node {
    virtual ~Node()=default;
    virtual void accept(Visitor&)=0;
};

struct MenuItem : Node {
    void accept(Visitor& v) override { v.visit(*this); }
};
struct MenuGroup : Node {
    Node* children[4]; int count=0;
    void accept(Visitor& v) override { v.visit(*this); }
};

struct RenderVisitor : Visitor {
    void visit(MenuItem&) override {/* draw leaf */}
    void visit(MenuGroup& g) override {/* draw group */}
};
```

## 6. 嵌入式适配要点
- 虚函数 + 多类增加尺寸
- 如果节点种类少（<=4），用枚举 + `switch` 更好
- 遍历栈深受限，使用迭代方式
- 构建期稳定则可用 `variant`

## 7. 进阶实现
### 7.1 variant 替代
```cpp
#include <variant>
struct MenuItem { /*...*/ };
struct MenuGroup { /*...*/ };
using Node = std::variant<MenuItem, MenuGroup>;

struct Render {
    void operator()(MenuItem&){ /* draw */ }
    void operator()(MenuGroup&){ /* draw */ }
};

void process(Node& n){
    std::visit(Render{}, n);
}
```
优点：无虚表；编译期分派；缺点：集合固定。

### 7.2 Acyclic Visitor（减少耦合）
解耦大 Visitor 接口 -> 复杂，不建议嵌入式小规模采用。

## 8. 资源与实时性评估
- 经典访问者：双虚调用（accept + visit 重载解析）
- variant：一次静态多态分支（可能生成跳转表）
- 尺寸：大量小 visit 函数可能内联

## 9. 替代比较
- `switch` + 枚举类型字段最简单
- 策略：用于替换行为族，不是跨多节点类型新增操作
- 模板泛型操作：对“概念”统一处理

## 10. 可测试性与可维护性
- 添加新操作：新增 Visitor 子类测试即可
- variant：测试 `std::visit` 结果；新增类型需改所有访问 lambda

## 11. 适用性检查清单
Use：
- 节点集合稳定 & 操作经常新增
Avoid：
- 节点类型经常增加/删除
- 类型数量小（switch 简单即可）

## 12. 常见误用 & 修正
误用：为仅两个类型添加一个新操作使用访问者。  
修正：直接函数重载或 `variant + visit`。

误用：访问者中修改大量内部私有字段破坏封装。  
修正：提供受控接口或只读操作。

## 13. 与其它模式组合
- 组合：树遍历 + 访问者操作
- 装饰器：访问前后添加统计
- 工厂：创建固定节点集合

## 14. 案例
菜单树统计：统计所有叶节点数量 + 最大深度操作独立实现，不修改节点类。

## 15. 实验建议
- 比较 1000 次访问 variant vs 访问者调用耗时
- .text 尺寸对比

## 16. MISRA / AUTOSAR 注意
- 多态访问需文档化
- variant 使用需确保访问完整性（所有类型分支覆盖）

## 17. 面试 / Review 问题
- 访问者 vs variant？
- 为何节点集合频繁变化时不适合？
- 如何减少样板（宏/模板）？

## 18. 练习
基础：实现 LoggingVisitor 统计访问次数。
提高：改用 variant 重写并比较尺寸。
思考：如何对 variant 集合新增类型时编译报错定位遗漏访问分支？

## 19. 快速回顾
- 将新操作与结构解耦
- 节点集合必须相对稳定
- variant 是现代可行替代
