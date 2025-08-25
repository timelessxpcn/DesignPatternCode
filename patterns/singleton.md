# 单例模式 (Singleton Pattern)

## 1. 意图
保证某对象在进程/系统生命周期内仅有一个实例，并提供全局访问点。

## 2. 问题背景 / 动机
嵌入式常见“全局硬件资源控制器”（时钟管理、日志路由、驱动复用）。初版用一堆 `extern` 全局变量，易出现：
- 初始化次序不确定（多个模块在使用前未初始化）
- 测试难以替换（硬编码全局）
- 资源重复初始化或未释放
单例被视作“简单解决”，但常引发耦合和隐藏依赖，需要谨慎与可控替代。

## 3. 适用场景
- 必须唯一的硬件抽象：SystemClock、PowerManager
- 访问昂贵但唯一的外设寄存器 API 封装
- 全局统计/日志（若可注入更佳）

不适用：
- 可以存在多个实例（传感器、通信通道）
- 仅为“方便访问”而非“语义唯一”

## 4. 结构概述
典型实现：私有构造 + 静态函数 `instance()` 返回静态局部对象 (Meyers Singleton)。嵌入式需关注：无异常、线程安全、启动顺序、可测试替换。

## 5. 基础实现（Meyers 单例，C++11 后线程安全）
```cpp
class SystemClock {
public:
    static SystemClock& instance() {
        static SystemClock inst;   // 初始化一次
        return inst;
    }
    void init() {
        if(!inited_) { /* 配置PLL/时钟 */ inited_ = true; }
    }
    uint32_t tick() const { return tick_; }
    void onTickISR() { ++tick_; } // 由 SysTick ISR 调用
private:
    SystemClock() = default;
    bool inited_{false};
    volatile uint32_t tick_{0};
};
```

## 6. 嵌入式适配要点
- 如果无多线程/多核，不必担心并发初始化。
- 若禁用全局静态初始化或使用自定义启动序列，可改为显式 `create()`。
- ISR 可使用单例对象时需保证其已初始化。
- 避免在构造中做大量硬件操作（潜在早期调用）；拆分 `init()`。

## 7. 进阶实现
### 7.1 显式生命周期控制 + 依赖注入 (可测试)
```cpp
class Clock {
public:
    using HWInitFn = void(*)();
    static void create(HWInitFn fn){
        inst_.hwInit_ = fn;
        inst_.hwInit_();
        inst_.ready_ = true;
    }
    static Clock& get(){ return inst_; }
private:
    HWInitFn hwInit_{nullptr};
    bool ready_{false};
    static Clock inst_;
};
Clock Clock::inst_{};
```
测试中调用 `Clock::create(mockFn)`。

### 7.2 可替换“伪单例”
通过接口 + 全局指针（默认实现）→ 单测替换为 Mock：
```cpp
struct IClock { virtual ~IClock()=default; virtual uint32_t tick() const=0; };
extern IClock* gClock; // 默认指向 RealClock
```

### 7.3 模板 Tag 限制多个逻辑单例
```cpp
template<typename Tag>
class Singleton {
public:
    static Singleton& instance(){ static Singleton s; return s; }
private: Singleton()=default;
};
struct ClockTag {};
using ClockMgr = Singleton<ClockTag>;
```

## 8. 资源与实时性评估
- 访问为一次函数调用 + 静态地址引用。
- 若包含锁（在 RTOS 环境）需评估临界区时间。
- Flash：极小；注意模板 Tag 泛化导致代码膨胀。

## 9. 与语言/库特性替代比较
- 命名空间 + 内部静态对象同等效果（封装度不同）
- 全局对象 + 明确 init() 调用更显式
- 依赖注入（构造参数传入）更易测试 & 解耦

## 10. 可测试性与可维护性
- 用可替换接口或 `create()` 注入 Mock
- 避免在单例内部访问其他单例（初始化循环依赖风险）
- 提供 `reset_for_test()`（仅测试编译宏）清理全局状态

## 11. 适用性检查清单
Use：
- 语义上确实唯一（硬件系统控制）
- 初始化需要延迟到首次使用
Avoid：
- 为避免传参偷懒
- 仅为管理配置数据（可放上下文对象）

## 12. 常见误用 & 修正
- 误用：到处 `System::instance()` 访问隐藏依赖 → 修正：以构造参数注入（显式）
- 误用：在构造函数中使用未初始化的其他单例 → 修正：拆分 init 阶段顺序调用
- 误用：单例持有巨大缓冲导致常驻内存浪费 → 修正：延迟分配 / 可选对象池

## 13. 与其它模式组合
- Facade：单例封装多子系统
- 工厂：单例工厂集中创建资源
- 命令：命令执行访问单例设备控制器（可通过接口抽象降低耦合）

## 14. 案例
`PowerManager` 单例：统一调度电压域、睡眠进入；测试用 Fake 实现统计调用。

## 15. 实验建议
- 测量直接全局 vs 单例访问开销（指令数）
- 静态分析循环依赖（使用 include graph）

## 16. MISRA / AUTOSAR 注意
- 全局可变状态需文档化
- 可考虑只读接口 + 限制写操作函数

## 17. 面试 / Review 问题
- 何时单例是反模式？
- 如何避免初始化顺序问题？
- 在多任务中如何实现线程安全单例？

## 18. 练习
基础：实现 Logger 单例（延迟初始化）。
提高：可替换 MockLogger（通过全局接口指针）。
思考：如何实现“仅初始化一次且可显式销毁”的单例（用于低功耗模式卸载）？

## 19. 快速回顾
- 谨慎使用，避免隐藏依赖
- 嵌入式重点：初始化时机与 ISR 安全
- 替代方案：注入 / 命名空间静态对象
