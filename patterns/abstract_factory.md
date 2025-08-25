# 抽象工厂模式 (Abstract Factory Pattern)

## 1. 意图
提供一个创建相关或依赖对象家族的接口，而无需指定具体类。

## 2. 问题背景 / 动机
嵌入式板卡可能存在“芯片族”差异：不同 SoC / 不同传感器 / 不同通信模块组合。直接在代码中混杂 `#ifdef CHIP_A ... #elif CHIP_B` 导致可读性差、测试困难。抽象工厂将“同一产品族”打包，按平台/配置选择一组具体实现。

## 3. 适用场景
- 多硬件平台：GPIO、I2C、SPI 驱动集合差异
- 功耗模式下使用不同资源策略集合
- 模拟 vs 真实硬件环境切换
- 协议族：加密 + 校验 + 打包 关联组件

## 4. 结构概述
AbstractFactory 定义创建一组产品接口（如 createGpio(), createSpi()）。ConcreteFactory 实现这些接口返回与平台匹配的具体产品对象。嵌入式为降低开销，可使用：
- 结构体内函数指针集合
- constexpr 静态表
- 模板参数选择特化

## 5. 基础实现（函数指针表）
```cpp
struct IGpio { virtual ~IGpio()=default; virtual void set(int pin,bool v)=0; };
struct ISpi  { virtual ~ISpi()=default; virtual void xfer(const uint8_t*, size_t)=0; };

struct PlatformFactory {
    IGpio* (*createGpio)();
    ISpi*  (*createSpi) ();
};

// 平台 A 实现
struct GpioA : IGpio { void set(int, bool) override {/*...*/} };
struct SpiA  : ISpi  { void xfer(const uint8_t*, size_t) override {/*...*/} };
IGpio* createGpioA(); ISpi* createSpiA();

constexpr PlatformFactory factoryA{
    &createGpioA, &createSpiA
};
```

## 6. 嵌入式适配要点
- 避免堆：返回静态对象指针或引用。
- 多工厂实例预分配在 ROM（`constexpr`）。
- 切换平台：编译期 (宏 / 模板) 或 运行期（探测 ID）。
- ISR 不适合调用创建；在初始化阶段一次性创建。

## 7. 进阶实现
### 7.1 模板标签 + 内联
```cpp
struct PlatformA{};
struct PlatformB{};

template<typename P> struct Factory;

template<> struct Factory<PlatformA> {
    static IGpio& gpio(){ static GpioA g; return g; }
    static ISpi& spi (){ static SpiA  s; return s; }
};
```
选择平台：`using ActivePlatform = PlatformA;`

### 7.2 产品族差异聚合结构
```cpp
struct Drivers {
    IGpio* gpio;
    ISpi*  spi;
    // ...
};
Drivers initDrivers(const PlatformFactory& f){
    return { f.createGpio(), f.createSpi() };
}
```

### 7.3 条件编译工厂合并
使用 `#if defined(BOARD_A)` 直接指向 `factoryA`，减少层级。

## 8. 资源与实时性评估
- 访问开销：函数指针一次间接或模板内联零开销。
- Flash：虚接口 + 产品代码；若使用模板特化可能重复实例化需关注尺寸。
- 初始化成本：一次性。

## 9. 替代比较
- 简单产品少：工厂方法即可。
- `variant`：当产品集合封闭且数目小，可组合在一起。
- 直接 `#ifdef`：小规模差异更直观；抽象工厂适合差异复杂/多产品联动。

## 10. 可测试性与可维护性
- 测试注入假工厂：返回 Mock 对象（统计调用）。
- 平台差异统一接口后可复用用例。
- 驱动家族扩展：新增一个 ConcreteFactory，不改业务代码。

## 11. 适用性检查清单
Use：
- 多个关联组件需一起切换
- 平台/环境差异复杂
Avoid：
- 只有单个组件差异（工厂方法即可）
- 运行期不需要切换（编译期宏足够）

## 12. 常见误用 & 修正
误用：为两个固定差异组件引入抽象层 + 虚函数浪费尺寸。  
修正：条件编译或策略模式分开处理。

误用：工厂中仍使用 `#ifdef` 分支创建所有产品。  
修正：每平台一个定义，选择工厂再统一访问。

## 13. 与其它模式组合
- 与单例：当前平台工厂单例暴露统一获取
- 与策略：某些产品内部行为再策略化
- 与对象池：工厂创建对象来自池

## 14. 案例
多板卡：BoardA 与 BoardB SPI/GPIO 参数不同；业务层使用 `Drivers d = initDrivers(activeFactory); d.spi->xfer(...);`

## 15. 实验建议
- 比较直接宏 vs 函数指针 vs 模板的 .text 尺寸
- 测量调用链指令数

## 16. MISRA / AUTOSAR 注意
- 函数指针使用需空指针检查
- 避免在初始化前访问产品对象

## 17. 面试 / Review 问题
- 与工厂方法区别？
- 如何减少抽象工厂带来的代码尺寸？ 
- 运行期 vs 编译期选择的权衡？

## 18. 练习
基础：添加 UART 产品接口与 PlatformA/B 实现。
提高：用模板特化实现编译期平台选择。
思考：如何在不增加运行期开销的前提下支持 4 平台？

## 19. 快速回顾
- 创建“相关对象族”统一入口
- 嵌入式需减少不必要抽象层
- 运行期/编译期两种选择路径
