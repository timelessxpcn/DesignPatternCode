# PImpl (Pointer to Implementation) 技术/习惯模式

## 1. 意图
通过将类的实现细节（成员变量、私有逻辑）隐藏在独立实现结构中，减少编译依赖、降低重编译、稳定 ABI，并可减小头文件暴露（接口最小化）。

## 2. 问题背景 / 动机
嵌入式项目增大后：  
- 改动一个头文件触发大量重编译  
- 需要隐藏硬件寄存器/第三方库头  
- 优化接口占用编译资源  
PImpl 将私有数据移至 `struct Impl` 中，头文件仅含前向声明，减少包含与耦合。

## 3. 适用场景
- 频繁修改实现细节但接口稳定的类
- 需要隐藏第三方库/硬件寄存器映射
- 控制编译时间 & 代码隔离
- 希望未来可替换实现而不改外部依赖

## 4. 结构概述
头文件：类声明 + `std::unique_ptr<Impl>`；源文件：定义 `struct Impl { ... }` 和构造/析构。嵌入式考虑堆问题，可使用静态缓冲 + placement new。

## 5. 基础实现（堆分配版）
```cpp
// driver.h
#pragma once
#include <memory>
class Driver {
public:
    Driver();
    ~Driver();
    bool init();
    int read();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// driver.cpp
#include "driver.h"
struct Driver::Impl {
    int fd;
    bool inited{false};
    int readHW(){ return 123; }
};
Driver::Driver(): impl_(new Impl{}) {}
Driver::~Driver() = default;
bool Driver::init(){ impl_->inited = true; return true; }
int Driver::read(){ return impl_->readHW(); }
```

## 6. 嵌入式适配要点
- 若禁用堆：使用固定大小内存缓冲。
- 增加内存占用（指针 + 可能的对齐填充）。
- 内联机会减少：访问成员需间接层。
- ABI 稳定（结构变化不影响头文件）。

## 7. 进阶实现
### 7.1 固定缓冲无堆
```cpp
// driver.h
class Driver {
public:
    Driver();
    bool init();
    int read();
private:
    struct Impl;
    alignas(Impl) unsigned char storage_[sizeof(Impl)];
    Impl* p();
    const Impl* p() const;
};

// driver.cpp
struct Driver::Impl { int fd; bool inited; int readHW(){ return 456;} };
Driver::Driver(){ new(storage_) Impl{0,false}; }
auto Driver::p()->Impl* { return reinterpret_cast<Impl*>(storage_); }
bool Driver::init(){ p()->inited = true; return true; }
int Driver::read(){ return p()->readHW(); }
```
### 7.2 按需延迟初始化
在首次调用 init() 时 placement new 创建 Impl，节省启动时间。

### 7.3 条件编译差异实现
`#if BOARD_A` 定义 ImplA / `#else` ImplB，统一指针类型隐藏差异。

## 8. 资源与实时性评估
- 每次访问多一层指针间接（可能影响热点性能）。
- Flash：Impl 代码移动到 .cpp，不一定减少总量。
- 减少重编译时间（对小固件项目意义有限，规模大才明显）。

## 9. 替代比较
- 普通封装：简单但暴露实现细节。
- 抽象接口 + 实例：更灵活多态，开销更大。
- 模块化 (C++20 模块) 将来也可替代部分 PImpl 需求。

## 10. 可测试性与可维护性
- 可在测试编译单元中访问 Impl（同一 .cpp）增加白盒测试（谨慎）。
- 更换实现不影响测试接口。
- 便于注入 Mock （若对外再暴露接口）。

## 11. 适用性检查清单
Use：
- 接口稳定、实现频繁变
- 编译时间痛点明显
Avoid：
- 小项目/超紧性能路径
- 禁止指针间接开销的硬实时内环

## 12. 常见误用 & 修正
误用：滥用 PImpl 使所有类间接访问增加复杂度。  
修正：仅在接口稳定 & 频繁修改类使用。

误用：Impl 里塞入所有数据成“上帝实现”。  
修正：分解子结构/模块。

## 13. 与其它模式组合
- Facade：Facade 使用 PImpl 隐藏复杂子系统。
- 代理：PImpl 内部可含代理控制访问。
- 抽象工厂：工厂生成不同 Impl 版本（需指针多态）。

## 14. 案例
设备驱动对外仅暴露 init/read，内部 Impl 包含硬件寄存器映射 struct（可能含 volatile），减少业务代码对寄存器头依赖。

## 15. 实验建议
- 比较添加 PImpl 前后修改 Impl 字段的重编译时间。
- 基准：循环调用 read() 查看指令差异。

## 16. MISRA / AUTOSAR 注意
- 强制转换 (reinterpret_cast) 审查。
- 不得访问未初始化 Impl 内存。

## 17. 面试 / Review 问题
- PImpl 与抽象接口差异？
- 如何在无堆下实现 PImpl？
- 何时不该使用 PImpl？

## 18. 练习
基础：将已有 Driver 改为无堆 PImpl。
提高：添加延迟初始化（首次 read 自动 init）。
思考：评估 PImpl 对性能的影响方式（工具/指标）。

## 19. 快速回顾
- 隐藏实现减少重编译耦合
- 嵌入式需评估指针间接 & 堆使用
- 只在有实际收益的复杂模块使用
