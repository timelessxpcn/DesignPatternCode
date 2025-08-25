# 原型模式 (Prototype Pattern)

## 1. 意图
通过克隆已有对象来创建新对象，而不是通过类构造器显式实例化，从而在运行期灵活复制配置/状态。

## 2. 问题背景 / 动机
嵌入式某些对象初始化代价高（配置寄存器、预计算表），需要多个相似实例（例如：不同通道的滤波器实例共享初始参数），直接重复初始化耗时或浪费能耗。原型模式以预先准备的“原型”对象复制创建，减少重复初始化逻辑。

## 3. 适用场景
- 需快速复制多份初始状态相同的配置对象
- 运行期根据已经调优的对象复制新实例
- 固件升级/回滚：保持配置快照（浅/深复制）
- 缓存“模板帧”再填差异字段

## 4. 结构概述
Prototype 基类定义 clone()；具体类实现深/浅复制。嵌入式中可能避开虚函数：使用结构体 + 显式复制函数或内存块复制（memcpy）+ 手动清理可变字段。

## 5. 基础实现（虚函数克隆）
```cpp
struct IFilter {
    virtual ~IFilter()=default;
    virtual IFilter* clone(void* buf) const = 0; // 无堆：放入外部缓冲
    virtual float apply(float v)=0;
};

struct LowPassFilter : IFilter {
    float alpha{0.5f};
    float state{0.f};
    IFilter* clone(void* buf) const override {
        return new(buf) LowPassFilter(*this); // 拷贝状态
    }
    float apply(float v) override {
        state = alpha * v + (1 - alpha) * state;
        return state;
    }
};
```

## 6. 嵌入式适配要点
- 避免堆：调用 clone 时传入外部预分配缓冲（对象池）。
- 明确浅/深复制：包含指针成员需复制数据或引用？
- 克隆后是否需要重置运行态（state）？可提供 cloneReset()。
- 使用 memcpy 需谨慎：含虚表指针对象不能直接裸拷贝。

## 7. 进阶实现
### 7.1 模板无虚函数原型
```cpp
template<typename T>
T* cloneObject(const T& src, void* buf){
    return new(buf) T(src); // 需确保拷贝构造行为正确
}
```
### 7.2 带清理策略
提供 hook：`T prepareClone(const T& src)` 重置运行中状态字段。

### 7.3 版本化原型表
维护数组 prototypes[]；索引 = 类型；快速克隆：常用于多种预设滤波配置。

## 8. 资源与实时性评估
- 克隆开销：拷贝构造 O(n) (字段数/大小)。
- 避免重复初始化：巨大预计算表复制 vs 引用共享（Flyweight 组合）。
- 额外存储：原型对象常驻内存。

## 9. 替代比较
- 直接构造：简单且无拷贝；当初始化便宜时优先。
- Builder：构建灵活但仍需要逐步设置。
- Flyweight：共享不可变部分；Prototype 复制可变部分。

## 10. 可测试性与可维护性
- 测试 clone 后对象行为独立（修改克隆不影响原型）。
- 测试深复制：内部指针数据修改是否隔离。
- 用对象池模拟多次克隆寿命。

## 11. 适用性检查清单
Use：
- 初始化昂贵或复杂
- 需要运行期快速复制带默认参数对象
Avoid：
- 对象很简单或复制成本与初始化接近
- 需要强类型安全而不愿引入虚函数（可用模板或直接构造）

## 12. 常见误用 & 修正
误用：浅复制导致共享内部缓冲数据竞争。  
修正：深复制或使用共享不可变 Flyweight。

误用：为简单 POD 使用虚克隆。  
修正：直接结构复制。

## 13. 与其它模式组合
- Builder：原型提供基础对象，Builder 再细化字段。
- Flyweight：共享不可变部分 + Prototype 克隆可变包装。
- 对象池：克隆目标存储在池内。

## 14. 案例
滤波器集合：原型包含已计算系数列表；克隆后只重置 state，缩短启动时间。

## 15. 实验建议
- 对比直接构造 vs 克隆启动耗时。
- 测试深复制与浅复制在指针成员对象修改后影响。

## 16. MISRA / AUTOSAR 注意
- 禁止使用未初始化缓冲放置 new。
- 指针成员复制策略文档化。

## 17. 面试 / Review 问题
- Prototype vs Builder 区别？
- 怎样避免浅复制风险？
- 对象池 + Prototype 的内存安全点？

## 18. 练习
基础：实现 cloneReset() 将运行态清零。
提高：添加共享不可变系数指针引用，克隆时引用不复制。
思考：如何保证 clone 在禁用 RTTI/异常下的错误处理？

## 19. 快速回顾
- 通过复制现有对象快速创建
- 嵌入式重点：无堆、浅/深复制明确
- 与 Flyweight/Builder 互补
