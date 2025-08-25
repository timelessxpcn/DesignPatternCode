# 状态模式 (State Pattern)

## 1. 意图
将一个对象在不同内部状态下的行为分离成独立状态类/单元，使状态转换与行为逻辑解耦，避免大量条件分支。

## 2. 问题背景 / 动机
嵌入式设备运行模式：Boot → Init → Running → LowPower → FirmwareUpdate。若用巨大 switch(state) 分发，逻辑易膨胀且修改风险高。状态模式将每种模式下的处理独立封装，支持清晰转换。

## 3. 适用场景
- 设备工作模式管理（功耗/运行/升级）
- 通信协议有限状态机（握手/认证/传输/错误恢复）
- 固件升级流程控制
- Menu / UI 导航（层级状态）

## 4. 结构概述
Context 持有指向 State 的指针或索引；事件驱动调用当前状态处理，必要时切换到其它状态（显式协定）。嵌入式可采用：函数指针表 / 枚举 + 数组 / 类封装（减少虚函数开销）。

## 5. 基础实现（函数指针表 + 枚举）
```cpp
enum class DevState { Boot, Init, Run, LowPower };

struct Context;
using StateFn = void(*)(Context&);

struct Context {
    DevState st = DevState::Boot;
    int counter = 0;
};

void onBoot(Context& c){
    // 初始化子系统...
    c.st = DevState::Init;
}
void onInit(Context& c){
    if (++c.counter > 3) c.st = DevState::Run;
}
void onRun(Context& c){
    // 正常工作; 某条件进入省电
    if (c.counter % 10 == 0) c.st = DevState::LowPower;
    ++c.counter;
}
void onLowPower(Context& c){
    // 等待唤醒
    c.st = DevState::Run;
}

static constexpr StateFn table[] = {
    onBoot, onInit, onRun, onLowPower
};

inline void dispatch(Context& c){
    table[static_cast<int>(c.st)](c);
}
```

## 6. 嵌入式适配要点
- 保证 dispatch O(1)，无堆。
- 状态数据可共享在 Context 里，避免频繁构造销毁。
- 若状态需要局部数据，可使用 union/variant 或单独结构并记录当前类型。
- ISR 内状态切换需考虑原子性（可能使用简单标志延迟到主循环）。

## 7. 进阶实现
### 7.1 类封装 + 接口（扩展性）
```cpp
struct IState {
    virtual ~IState() = default;
    virtual void step() = 0;
};

class Device {
public:
    void setState(IState* s){ state_ = s; }
    void loop(){ state_->step(); }
private:
    IState* state_;
};
```
使用静态单例状态对象：减少创建开销。

### 7.2 编译期状态机（constexpr 转换表）
使用 `constexpr` 二维数组定义状态-事件转换，事件驱动快速查表。

### 7.3 分层状态（Sub-State）
主状态如 Run 下再细分 (Normal / Warning / Error)，可嵌套 dispatch，避免平铺指数增长。

## 8. 资源与实时性评估
- 函数指针表：单次间接调用。
- 虚函数实现：增加虚表访问。
- 代码尺寸：状态越多越增长，可利用宏/生成脚本统一管理。
- 时间：O(1) 转换。注意状态进入/离开耗时（初始化设备等需分拆异步）。

## 9. 替代比较
- 简单 2~3 状态可直接 if/switch。
- 模板 + CRTP 对编译期已知状态集合执行内联优化（但扩展需重编译）。
- 表驱动 FSM：更适合大量均匀事件处理。

## 10. 可测试性与可维护性
- 测试转换路径：构造初始上下文，注入事件序列，断言最终状态。
- 覆盖所有合法和错误转换（尝试无效事件）。
- 将硬件副作用隔离在状态进入函数，可模拟替换。

## 11. 适用性检查清单
Use：
- 状态数 > 3 且会扩展
- 行为随状态变化明显
Avoid：
- 行为差异极小（switch 更直观）
- 状态频繁进入/退出伴随大开销初始化（可加缓存或拆分）

## 12. 常见误用 & 修正
误用：过度拆分，微小条件差异也建独立状态 → 类爆炸。  
修正：合并相似行为，用少量条件处理细微差异。

## 13. 组合
- 与策略：状态内选择不同策略微调算法。
- 与命令：状态转换触发一系列命令排队执行。
- 与观察者：外部模块订阅状态变化事件。

## 14. 案例
功耗管理：Run 状态周期性评估负载→切换 LowPower；接收外部唤醒中断 Flag 后下一轮主循环切回 Run。

## 15. 实验建议
统计 dispatch 周期 jitter：比较 switch vs 函数指针 vs 虚函数实现。  
测量 flash 差异：nm/size 工具统计。

## 16. MISRA / AUTOSAR 注意
- 枚举转换需完整 default 分支（如果使用 switch）。
- 函数指针数组索引安全（边界检查或静态断言）。

## 17. 面试 / Review 问题
- State vs Strategy 差异（关注内部状态 vs 可替换算法）。
- 如何避免状态膨胀？
- 进入/退出耗时大的状态如何平滑处理？

## 18. 练习
基础：添加 FirmwareUpdate 状态（分块写入模拟）。
提高：实现表驱动事件 → 状态转换（二维数组）。
思考：在多核/多任务系统中防止并发状态切换冲突方法？

## 19. 快速回顾
- 封装随状态变化的行为
- 函数指针表高效无堆
- 避免不必要细粒度状态拆分
