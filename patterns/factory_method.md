# 工厂方法 (Factory Method)

## 1. 意图
通过延迟对象具体创建到子类/独立创建函数，使调用方只依赖抽象产品接口，实现“选择哪种具体实现”与“使用”解耦。

## 2. 问题背景 / 动机
嵌入式项目中常需支持多种硬件模块（不同 SPI 传感器 / 不同通信模块）。最初直接在高层 if-else new 具体驱动，修改添加新型号会触及核心逻辑，且难以在单测替换为虚拟设备。工厂方法将创建逻辑集中与可扩展点分离。

## 3. 适用场景
- 多型号外设（传感器A/B/C）的抽象封装
- 根据编译宏/配置结构选择驱动实现
- 测试环境使用模拟/回放设备替换真实硬件
- 协议栈不同链路层实现切换

## 4. 结构概述
Creator（工厂接口或基类）暴露 createX()；具体 Creator 决定返回哪个 ConcreteProduct。嵌入式中常不引入额外子类：使用自由函数 + 注册表 / 条件编译即可达到相同目的。

## 5. 基础实现（经典虚函数版本）
```cpp
#include <memory>
#include <cstdint>

struct Sensor {
    virtual ~Sensor() = default;
    virtual int read() = 0;
};

struct SensorA : Sensor {
    int read() override { return 42; } // 模拟
};
struct SensorB : Sensor {
    int read() override { return 100; }
};

struct SensorFactory {
    virtual ~SensorFactory() = default;
    virtual std::unique_ptr<Sensor> create(uint8_t id) = 0;
};

struct SimpleSensorFactory : SensorFactory {
    std::unique_ptr<Sensor> create(uint8_t id) override {
        if (id == 0) return std::make_unique<SensorA>();
        if (id == 1) return std::make_unique<SensorB>();
        return nullptr;
    }
};
```

## 6. 嵌入式适配要点
- 避免频繁堆分配：可返回放在静态区/对象池的指针或引用。
- 在启动阶段（init）完成创建；运行期循环避免创建销毁。
- 不必强求面向对象：若产品族简单，用 switch + 结构体表更直接。
- 中断中不调用工厂（非确定 / 可能分配）。

## 7. 进阶实现
### 7.1 无堆静态存储版本
```cpp
class StaticSensorFactory {
public:
    Sensor* create(uint8_t id) {
        switch(id){
        case 0: return &a_;
        case 1: return &b_;
        default: return nullptr;
        }
    }
private:
    SensorA a_;
    SensorB b_;
};
```
优点：无动态分配；缺点：所有实例常驻内存。

### 7.2 模板 + 策略注册（编译期选择）
```cpp
template<typename... Sensors>
struct SensorList {};

template<uint8_t ID, typename T>
struct MapEntry { static constexpr uint8_t id = ID; using Type = T; };

template<typename... Entries>
class CTFactory {
public:
    static Sensor* create(uint8_t id){
        Sensor* result = nullptr;
        ((id == Entries::id ? (result = &get<Entries::Type>()) : (void)0), ...);
        return result;
    }
private:
    template<typename T>
    static T& get(){ static T instance; return instance; }
};
using MyFactory = CTFactory<MapEntry<0, SensorA>, MapEntry<1, SensorB>>;
```
优点：无虚函数，无分配。缺点：扩展需改代码重新编译。

### 7.3 条件编译 + 配置结构
```cpp
struct SensorConfig { uint8_t type; /* ... */ };

Sensor* createSensor(const SensorConfig& cfg){
#if defined(HAVE_SENSOR_A)
    if (cfg.type == 0) static SensorA a; return &a;
#endif
#if defined(HAVE_SENSOR_B)
    if (cfg.type == 1) static SensorB b; return &b;
#endif
    return nullptr;
}
```
适合裁剪目标可执行体体积。

## 8. 资源与实时性评估
- 创建阶段成本：一次性；建议在系统初始化阶段完成。
- 运行时：调用产品接口不应再有分支（依赖多态或直接对象表）。
- 模板方式：代码尺寸略增（展开 id 匹配逻辑）。

## 9. 与语言/库特性替代比较
- 直接 if/switch：当产品种类少且稳定。
- variant：若所有产品方法统一且需要 visit，不必抽象基类。
- 简单结构体数组 + 函数指针：极小开销方案。

## 10. 可测试性与可维护性
- 工厂注入“模拟器”实现（传感器返回固定或脚本读数）。
- 通过接口隔离避免业务代码直接依赖具体硬件寄存器。
- 提供 createFromDescriptor(config) 统一入口，便于测试构造。

## 11. 适用性检查清单
使用：
- 需在不同编译配置 / 运行配置下选择实现
- 需要单测替换真实硬件
不使用：
- 单一实现且无扩展计划
- 初始化开销微不足道 + 直接构造更直白

## 12. 常见误用 & 修正
误用：为 2 种固定硬件写一层抽象 + 虚函数 + 动态分配 → 复杂度与尺寸不成比例。  
修正：使用 constexpr 配置或简单 switch 返回静态实例。

## 13. 与其它模式组合
- 与策略：工厂生产策略对象（功耗/编码）
- 与对象池：工厂管理池中可重用对象
- 与抽象工厂：该模式可视为抽象工厂的精简单产品版本

## 14. 真实或模拟案例
多型号温度传感器（I2C 地址不同 / 校准算法不同），工厂根据探测到的 WHO_AM_I 寄存器选择驱动实例。

## 15. 性能/尺寸实验建议
比较：
- 动态分配 vs 静态实例（.bss/.data/.text 大小）
- 工厂调用次数（初始化 vs 运行期）对总体 CPU 周期影响（应接近 0）。

## 16. MISRA / AUTOSAR 注意
- 若使用动态分配需有受控生命周期说明
- 避免在工厂函数中过度使用可变参数/宏隐藏复杂逻辑

## 17. 面试 / Review 问题
- 什么时候工厂是“过度设计”？
- variant + visit 与 工厂 + 多态的取舍？
- 如何在无 RTTI 环境做创建选择？

## 18. 练习
基础：新增 SensorC（id=2），实现静态工厂支持。  
提高：实现对象池版工厂：限制最多 2 个相同传感器实例。  
思考：如何在硬件探测失败时安全降级到模拟器？

## 19. 快速回顾
- 解耦“创建”与“使用”
- 嵌入式更注重创建阶段集中化与静态实例
- 可被简单 switch / variant / 条件编译替代
