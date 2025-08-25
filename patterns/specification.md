# 规格模式 (Specification Pattern)

## 1. 意图
用可组合的布尔逻辑对象（规格）描述业务/过滤条件，通过 AND/OR/NOT 组合形成复杂判定，避免大量硬编码 if 条件散落。

## 2. 问题背景 / 动机
嵌入式例如过滤事件：电压区间、温度阈值、时间窗口、标志位；组合逻辑经常变化。直接写 if (a && (b || c) && !d) 可读性差且难测试每个子条件。Specification 将条件封装对象，可单测与复用。

## 3. 适用场景
- 事件过滤/报警规则
- 功耗决策条件（负载 & 温度 & 定时）
- 输入信号校验（范围 + 黑名单 ID）
- 数据采样触发 (复合条件)

## 4. 结构概述
Specification 抽象 has `isSatisfiedBy(obj)`；具体规格实现；组合规格 (And/Or/Not) 持有子规格引用。嵌入式避免动态分配：使用静态对象 + 引用。

## 5. 基础实现（虚接口）
```cpp
template<typename T>
struct ISpec {
    virtual ~ISpec()=default;
    virtual bool ok(const T& v) const = 0;
};

template<typename T>
struct RangeSpec : ISpec<T> {
    T min_, max_;
    bool ok(const T& v) const override { return v >= min_ && v <= max_; }
};

template<typename T>
struct AndSpec : ISpec<T> {
    const ISpec<T>& a; const ISpec<T>& b;
    AndSpec(const ISpec<T>& x, const ISpec<T>& y):a(x),b(y){}
    bool ok(const T& v) const override { return a.ok(v) && b.ok(v); }
};
```

## 6. 嵌入式适配要点
- 虚调用开销若敏感可用模板组合。
- 避免堆：静态或栈对象组合。
- 条件集合大时可能深层调用：可预编译合并条件。
- 在 ISR 中执行时确保条件函数快速（内联/模板版）。

## 7. 进阶实现
### 7.1 模板静态组合
```cpp
template<typename F1, typename F2>
struct AndT {
    F1 f1; F2 f2;
    template<typename T>
    bool ok(const T& v) const { return f1.ok(v) && f2.ok(v); }
};
```
### 7.2 策略数据驱动
规格参数来自配置表（阈值、标志位掩码），运行期重构组合。

### 7.3 编译期常量折叠
使用 constexpr 规格将恒真/恒假优化掉。

## 8. 资源与实时性评估
- 虚版：每层 And/Or 一次间接。
- 模板版：内联展开，代码膨胀随组合增长。
- 数据驱动：解析开销在更新时发生。

## 9. 替代比较
- 直接 if：简单条件足够。
- 策略模式：策略选择 vs 规格组合布尔表达式。
- 责任链：处理请求并可能终止；规格仅判断。

## 10. 可测试性与可维护性
- 每个基本规格单元单测。
- 组合规格 -> 真值表测试。
- 动态参数更新：测试重载后行为。

## 11. 适用性检查清单
Use：
- 复杂且变化条件集合
- 条件希望复用 & 可单测
Avoid：
- 条件简单且稳定
- 阶段性能极限不容许多层调用（改模板内联或合并表达式）

## 12. 常见误用 & 修正
误用：深度嵌套规格链导致调用开销。  
修正：扁平化/合并常量表达式。

误用：规格中混入副作用（写数据）。  
修正：规格只读。

## 13. 与其它模式组合
- Builder：构造复合规格对象集合。
- 责任链：先用规格筛选，再进入链。
- 命令：规格判定后生成命令。

## 14. 案例
功耗策略：CPUUsageSpec(0-40%) AND TempSpec(20-60C) AND NOT NightModeSpec -> 进入 Normal；否则降频。

## 15. 实验建议
- 比较 if 写死 vs 规格虚调用 vs 模板静态组合耗时。
- 度量规格数量增加对 Flash 增量。

## 16. MISRA / AUTOSAR 注意
- 布尔函数副作用禁止。
- 使用内联/constexpr 确保可预测。

## 17. 面试 / Review 问题
- 与策略模式区别？
- 如何避免组合导致性能回退？
- 动态参数更新一致性？

## 18. 练习
基础：实现 NotSpec / OrSpec。
提高：实现模板静态组合 + 恒真折叠。
思考：如何将配置表解析为规格组合结构并可热更新？

## 19. 快速回顾
- 可组合布尔表达式对象化
- 虚 vs 模板取舍：灵活 vs 零开销
- 避免过度抽象简单条件
