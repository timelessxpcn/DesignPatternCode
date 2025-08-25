# 适配器模式 (Adapter Pattern)

## 1. 意图
在不修改既有客户端代码与被适配组件的情况下，转换接口以实现复用与解耦。

## 2. 问题背景 / 动机
嵌入式项目中引入第三方驱动或遗留 C API（函数 `legacy_read_temp()`），而系统上层统一依赖 `ISensor` 接口。直接改遗留代码风险高或不可控（闭源库 / 合规限制），适配器在两者之间桥接，避免大范围重写。

## 3. 适用场景
- 统一多种不同寄存器布局的传感器读取接口
- 把旧 C 驱动函数集合包装成面向对象接口
- 将阻塞式 API 包装为轮询或非阻塞调用（通过缓存上次结果）
- 不同通信协议（I2C/SPI/UART）下暴露统一高层接口

## 4. 结构概述
Client -> Target(抽象) <- Adapter 包装 Adaptee(遗留对象/函数集)。  
可分：对象适配（保存指针）/ 类适配（多继承，较少在嵌入式使用）/ 函数式适配（lambda 包装）。

## 5. 基础实现（对象适配，包装 C 函数）
```cpp
extern "C" {
    int legacy_init();
    int legacy_read_temp_cx10(); // 返回温度 *10
}

struct ISensor {
    virtual ~ISensor() = default;
    virtual float readCelsius() = 0;
};

class LegacyTempAdapter : public ISensor {
public:
    LegacyTempAdapter() { legacy_init(); }
    float readCelsius() override {
        int v = legacy_read_temp_cx10();
        return v / 10.0f;
    }
};
```

## 6. 嵌入式适配要点
- 避免在 ISR 中调用初始化（可能内部使用不可重入资源）。
- 若适配阻塞接口，需要注明最大阻塞时间（对实时性影响）。
- 可通过预取 + 缓存时间戳减少读取频率。
- 若无堆，Adapter 对象放静态区或栈中。

## 7. 进阶实现
### 7.1 缓存 + 轮询适配
```cpp
class CachedTempAdapter : public ISensor {
public:
    float readCelsius() override {
        uint32_t now = tickMs();
        if (now - lastUpdate_ > 100) {
            lastValue_ = rawRead();
            lastUpdate_ = now;
        }
        return lastValue_;
    }
private:
    float rawRead() { return legacy_read_temp_cx10() / 10.0f; }
    uint32_t lastUpdate_ = 0;
    float lastValue_ = 0.f;
};
```
### 7.2 编译期函数指针适配（无虚函数）
```cpp
template<int(*RawFn)()>
struct TempAdapterT {
    float readCelsius() const { return RawFn() / 10.0f; }
};
```

## 8. 资源与实时性评估
- 适配层增加一次调用间接（虚函数或普通函数）。
- 缓存策略可减少昂贵硬件访问，改善功耗与实时占用。
- 模板适配消除虚调用，但会为每个 RawFn 生成代码体。

## 9. 与语言/库特性替代比较
- 简单场景可使用 lambda 包装转换：`auto f=[&]{ return legacy()/10.0f; };`
- 若只需一次转换，不必建立类结构（避免过度抽象）。
- 若需双向转换（协议翻译），可能更适合 Facade + Adapter 组合。

## 10. 可测试性与可维护性
- 将 `legacy_read_temp_cx10()` 抽象为函数指针参数，单测传入模拟函数。
- 缓存逻辑测试：注入 tick 模拟时间推进。

## 11. 适用性检查清单
Use：
- 不可修改的遗留/外部接口
- 需要统一上层接口
Avoid：
- 可以直接修改源代码
- 额外包装只增加调用层且无复用价值

## 12. 常见误用 & 修正
误用：为已能修改的内部模块写 Adapter，导致结构冗余。  
修正：直接重构原模块接口或使用轻量 inline 包装。

## 13. 与其它模式组合
- 与装饰器：适配后再添加校准/滤波。
- 与工厂：按设备探测结果创建正确适配器。
- 与策略：不同适配器作为策略实现集合。

## 14. 案例
统一不同加速度计（比例/偏移不同）到 `ISensor::readG()` 接口，适配器内做缩放与校准。

## 15. 性能/尺寸实验建议
对比直接调用 legacy vs Adapter（虚函数）在 1e6 次循环的耗时差异。统计闪存增加（nm / size 工具）。

## 16. MISRA / AUTOSAR 注意
- 包装 C 接口需确保 extern "C" 一致。
- 避免在构造函数中多阶段可能失败初始化（分拆 init()）。

## 17. 面试 / Review 问题
- 如何判断使用 Adapter 而不是直接修改？
- 如何减少多层适配链造成的延迟？
- 缓存适配的失效策略设计？

## 18. 练习
基础：为返回华氏度的 legacy API 适配为摄氏。
提高：实现一个组合适配器把 3 个子传感器数据聚合为结构体。
思考：如果底层读取阻塞 5ms，如何最小化对 1ms 调度循环的影响？

## 19. 快速回顾
- 解决“接口不匹配”而非“行为族扩展”
- 嵌入式重点：阻塞/缓存/不可修改 C API
- 避免不必要适配层
