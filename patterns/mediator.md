# 中介者模式 (Mediator Pattern)

## 1. 意图
通过引入中介对象封装对象之间的交互，避免组件相互直接引用产生网状耦合，集中管理通信逻辑。

## 2. 问题背景 / 动机
多个模块（传感器、通信、功耗、显示）彼此互调：A 通知 B 更新，B 又调用 C，逐渐形成依赖网。功能添加/修改一处牵动多处且难以单测。中介者将消息/命令路由集中某个模块，实现松耦合与更清晰的拓扑。

## 3. 适用场景
- 模块间需要广播/路由命令但不想建立多重依赖
- 简化多组件交互顺序（启动/关闭协调）
- 资源有限时集中做优先级仲裁（谁先执行）
- 动态启停组件时减少互相指针更新

## 4. 结构概述
Colleague（各组件）只与 Mediator 接口交互；Mediator 决定转发/调度。与观察者不同：中介者更强调“多方协作逻辑集中”和“决策控制”，而非简单事件广播。

## 5. 基础实现（简易路由）
```cpp
enum class MsgType : uint8_t { SensorData, PowerChange, DisplayReq };

struct Message {
    MsgType type;
    uint32_t param;
};

class IMediator {
public:
    virtual ~IMediator()=default;
    virtual void send(const Message& m, void* sender)=0;
};

class MediatorSimple : public IMediator {
public:
    void registerSensor(void* s){ sensor_ = s; }
    void registerDisplay(void* d){ display_ = d; }
    void registerPower(void* p){ power_ = p; }

    void send(const Message& m, void* sender) override {
        switch(m.type){
        case MsgType::SensorData:
            // 转发给显示/功耗
            if(display_) handleDisplay(m);
            if(power_) handlePower(m);
            break;
        case MsgType::PowerChange:
            if(sensor_) handleSensorPower(m);
            break;
        default: break;
        }
    }
private:
    void* sensor_{nullptr};
    void* display_{nullptr};
    void* power_{nullptr};
    void handleDisplay(const Message&){ /* ... */ }
    void handlePower(const Message&){ /* ... */ }
    void handleSensorPower(const Message&){ /* ... */ }
};
```

## 6. 嵌入式适配要点
- O(1)/O(n) 决策：组件数较少直接 switch 即可。
- 避免中介者膨胀成为“上帝对象”→ 抽出策略或子 Mediator。
- 可将消息结构压缩（位域）节省带宽。
- ISR 中转发：避免复杂逻辑，记录事件，主循环处理。

## 7. 进阶实现
### 7.1 发布订阅桥接
Mediator 内部持有 Subject/Observer（组合模式） → 将复杂广播交给已有观察者模式。
### 7.2 优先级调度
消息队列（优先级队列）+ 调度策略（策略模式）决定处理顺序。
### 7.3 分层中介
高层系统中介 -> 子系统中介（通信/传感器），减少单点复杂度。

## 8. 资源与实时性评估
- 额外转发层：一次函数调用 + 分支。
- 集中调度可能引入延迟队列：需容量评估。
- 切勿在 Mediator::send 中执行长耗时阻塞。

## 9. 替代比较
- 直接调用：组件少且关系简单更直白。
- 观察者：广播逻辑但不做复杂决策。
- 事件总线（全局）：更松散但缺少上下文协调。

## 10. 可测试性与可维护性
- 可以注入假组件（指针接口或者函数指针）验证消息流。
- 中介者单元测试：给定输入消息 -> 断言触发路径。
- 统计/日志在一个点集中易监控。

## 11. 适用性检查清单
Use：
- 组件互相调用网状增长
- 需要集中协调顺序/优先级
Avoid：
- 组件数少，交互简单
- 中介者仅 pass-through 没有价值（引入层浪费）

## 12. 常见误用 & 修正
误用：所有操作都经过中介者，导致巨大类。  
修正：拆分多个主题/子中介或用策略拆分处理函数。

误用：中介者隐藏过多业务逻辑（难测试）。  
修正：业务处理放在组件内，中介者只路由。

## 13. 与其它模式组合
- 观察者：中介者内部使用观察者广播
- 命令：消息封装成命令排队
- 策略：调度策略可热切换

## 14. 案例
功耗管理：Sensor -> Mediator -> 确定是否需要 Display 更新与 Power 模式变更，集中执行顺序（先停显示再降频）。

## 15. 实验建议
- 比较网状调用 vs 中介者路由的代码行与耦合度（include 数量）。
- 测量转发延迟（消息生产到执行）。

## 16. MISRA / AUTOSAR 注意
- 指针有效性检查
- 枚举 switch 覆盖所有路径

## 17. 面试 / Review 问题
- Mediator 与 Observer 区别？
- 如何防止中介者膨胀？
- 优先级处理策略设计？

## 18. 练习
基础：添加 DisplayReq 消息处理。
提高：实现优先级队列（高优先级先执行）。
思考：多核场景下中介者同步策略？

## 19. 快速回顾
- 降低网状耦合
- 适度集中协调/优先级
- 避免膨胀为上帝对象
