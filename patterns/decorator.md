# 装饰器模式 (Decorator Pattern)

## 1. 意图
在不改变原始对象接口的前提下，动态或静态地为对象添加职责（功能扩展/包装）。

## 2. 问题背景 / 动机
例如传感器读取需要“基础读取 + 滤波 + 校准 + 记录最大值”多层逻辑。初版在单一函数中不断添加代码（难以维护/测试）。装饰器允许将每层独立封装组合。

## 3. 适用场景
- 传感器读取：滤波 → 校准 → 限幅
- 日志输出：格式化 → 级别过滤 → 输出设备
- 通信报文处理：加密 → 压缩 → 校验
- 数据流处理链可变

## 4. 结构概述
Component 接口；ConcreteComponent；Decorator 持有 Component 指针并转发+增强。嵌入式可：
- 静态组合（模板继承/聚合）减少堆
- 编译期流水线（模板参数包）

## 5. 基础实现（指针链）
```cpp
struct ISensor {
    virtual ~ISensor()=default;
    virtual float read()=0;
};

struct RawSensor : ISensor {
    float read() override { return 42.0f; } // 模拟
};

struct SensorDecorator : ISensor {
    explicit SensorDecorator(ISensor* inner):inner_(inner){}
protected:
    ISensor* inner_;
};

struct ClampDecorator : SensorDecorator {
    using SensorDecorator::SensorDecorator;
    float read() override {
        float v = inner_->read();
        if(v>50) v=50; if(v<0) v=0;
        return v;
    }
};
```

## 6. 嵌入式适配要点
- 避免深层虚调用链（多级间接）。
- 对性能敏感可使用模板静态装饰（CRTP 或函数内联）。
- 考虑栈/静态分配：链在初始化阶段构建。
- 若所有装饰顺序固定：使用简单函数组合或流水线数组更轻量。

## 7. 进阶实现
### 7.1 模板流水线
```cpp
template<typename... Layers>
struct Pipeline;

template<typename L1, typename... Rest>
struct Pipeline<L1, Rest...> {
    L1 layer; Pipeline<Rest...> next;
    float read(){ return next.read(layer.read()); }
};

template<>
struct Pipeline<>{
    float read(float v){ return v; }
};
```
（实际需定义每层 `read(float)` 接口）

### 7.2 函数数组链
```cpp
using FilterFn = float(*)(float);
float apply(float in, FilterFn* fns, size_t n){
    for(size_t i=0;i<n;++i) in = fns[i](in);
    return in;
}
```

### 7.3 条件编译层
根据宏包含或排除某些装饰层减少代码尺寸。

## 8. 资源与实时性评估
- 链长度 * 虚调用 = 额外延迟
- 模板流水线：内联优化但代码膨胀
- 函数数组：循环 + 间接 pointer 成本低

## 9. 替代比较
- 直接顺序调用多个处理函数（无需模式）
- 策略模式：替换单个算法，不是层叠
- 责任链：强调请求可被拦截终止；装饰器总是继续下一个

## 10. 可测试性与可维护性
- 单独测试每个装饰层
- 组合测试：确认顺序影响正确（顺序不同可能结果差异）
- 提供构建函数统一组装链

## 11. 适用性检查清单
Use：
- 可插拔、可组合处理层
- 层数变化或受配置控制
Avoid：
- 层集合固定且简单（函数相加即可）
- 性能极限路径（改静态内联）

## 12. 常见误用 & 修正
误用：大量微小装饰层（函数体 1-2 行），增加深度。  
修正：合并微小层。

误用：装饰中修改语义（非“附加”），破坏调用者预期。  
修正：限制装饰责任只增强不改变核心协定。

## 13. 与其它模式组合
- 工厂：按配置构建装饰链
- 策略：某层内部使用策略算法
- 观察者：在装饰层内发送通知

## 14. 案例
温度读取：Raw -> 滑动平均 -> 校准曲线 -> 夹紧 → 输出。

## 15. 实验建议
- 比较 4 层虚链 vs 函数数组 vs 模板流水线耗时和 flash
- 链层数对 jitter 的影响

## 16. MISRA / AUTOSAR 注意
- 虚函数层级文档化
- 函数指针数组需空指针检查

## 17. 面试 / Review 问题
- 装饰器 vs 责任链区别？
- 如何减少多层虚调用开销？
- 如何在配置裁剪中去掉某层？

## 18. 练习
基础：添加滑动平均装饰。
提高：实现函数数组链 + 动态配置层顺序。
思考：在硬实时约束中选择哪种装饰实现方式与理由？

## 19. 快速回顾
- 动态/静态添加职责
- 嵌入式需取舍虚调用 vs 模板膨胀
- 组合构建可由工厂统一管理
