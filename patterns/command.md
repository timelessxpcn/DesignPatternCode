# 命令模式 (Command Pattern)

## 1. 意图
将“请求”封装为对象，使调用者与执行者解耦，并可支持排队、撤销、记录、延时执行等功能。

## 2. 问题背景 / 动机
嵌入式中多个模块需要向执行引擎（电机控制 / 外设操作 / 配置写入）发起操作。初版直接调用函数，难以：
- 在 ISR 中排队稍后执行
- 记录/回放命令序列
- 插入权限或速率限制
命令模式将操作与参数封装为统一接口放入队列，主循环或专用任务统一调度。

## 3. 适用场景
- ISR 生成操作请求（如“发送报文”）主循环延迟执行
- 日志/配置命令回放
- 电机/执行器动作排队 + 取消
- 远程控制指令映射

## 4. 结构概述
Command 抽象：`execute()`；Invoker(调度者) 持有队列；Receiver(实际执行者) 由命令内部引用或注入。嵌入式中常去除虚函数，使用函数指针或枚举 + 联合参数结构。

## 5. 基础实现（枚举 + 联合紧凑表示）
```cpp
enum class CmdType : uint8_t { SetSpeed, SetMode };

struct CmdSetSpeed { uint16_t rpm; };
struct CmdSetMode  { uint8_t mode; };

struct Command {
    CmdType type;
    union {
        CmdSetSpeed speed;
        CmdSetMode  mode;
    } data;
};

template<size_t N>
class CommandQueue {
public:
    bool push(const Command& c){
        size_t next = (w_+1)%N;
        if(next==r_) return false;
        buf_[w_] = c; w_=next; return true;
    }
    bool pop(Command& c){
        if(r_==w_) return false;
        c = buf_[r_]; r_=(r_+1)%N; return true;
    }
private:
    Command buf_[N];
    size_t w_{0}, r_{0};
};

struct Motor {
    void setSpeed(uint16_t rpm){ /* ... */ }
    void setMode (uint8_t m){ /* ... */ }
};

inline void executeCommand(Motor& m, const Command& c){
    switch(c.type){
    case CmdType::SetSpeed: m.setSpeed(c.data.speed.rpm); break;
    case CmdType::SetMode:  m.setMode (c.data.mode.mode); break;
    }
}
```

## 6. 嵌入式适配要点
- 无堆：队列使用静态环形缓冲。
- ISR：构造命令结构并 push（需 O(1)）。满队列策略要定义（丢弃/覆盖/计数）。
- 封装大小与对齐：减小命令结构以节省 RAM。
- 可选 CRC/校验字段防止内存破坏导致执行危险操作。

## 7. 进阶实现
### 7.1 对象池 + 多态命令
```cpp
struct ICommand { virtual ~ICommand()=default; virtual void exec()=0; };

template<size_t N>
class CommandPool {
    // 复用 object_pool 模板思想 (省略实现)
};
```
适合参数差异大的命令；代价：虚表/尺寸。

### 7.2 函数指针 + 上下文
```cpp
struct FnCommand {
    void (*fn)(void*, uint32_t);
    void* ctx;
    uint32_t arg;
};
```
统一签名减少结构体尺寸。

### 7.3 撤销 (Undo)
存储逆操作参数：如 SetSpeed(oldRpm)；执行后压入撤销栈。

## 8. 资源与实时性评估
- push/pop O(1)。
- 命令处理时间由执行函数决定，应避免长阻塞；必要时拆分成小命令链。
- 队列容量决定最大待处理积压。

## 9. 替代比较
- 直接函数调用：简单但缺少排队/记录。
- 观察者：广播事件；命令更强调“动作执行”而非“通知”。
- 任务消息队列 (RTOS)：系统级实现，可代替自定义队列。

## 10. 可测试性与可维护性
- 构造命令序列，执行后检查 Receiver 状态。
- 模拟队列满情况。
- 逆操作测试（如果支持撤销）。

## 11. 适用性检查清单
Use：
- 需要排队 / 延时 / 记录
- ISR 需快速排队操作
Avoid：
- 单一立即执行操作
- 内存极度紧张（命令结构额外开销不值）

## 12. 常见误用 & 修正
误用：命令过度细粒度（大量 trivial 命令造成调度开销 > 实际工作）。  
修正：合并多次小操作成批处理命令。

误用：忽略队列满导致静默丢失关键命令。  
修正：返回状态 + 统计指标 + 背压策略。

## 13. 与其它模式组合
- 与对象池：命令对象复用
- 与状态机：状态决定是否接受/排队新命令
- 与策略：执行策略影响命令处理顺序（优先级策略）

## 14. 案例
远程控制帧解析 → 生成命令 (SetSpeed)，ISR 放入队列；主循环按照优先级处理；若队列接近满则丢弃低优先级命令并计数。

## 15. 实验建议
- 测试不同命令结构尺寸对整体 RAM 占用。
- 基准：100k 次 push/pop 循环耗时。
- 模拟高负载下丢弃率统计。

## 16. MISRA / AUTOSAR 注意
- 使用 union 访问须确认活跃成员正确。
- switch 必须包含 default（如标准要求覆盖）。

## 17. 面试 / Review 问题
- 与事件/观察者区别？
- 如何添加优先级调度？
- 撤销机制的内存策略设计？

## 18. 练习
基础：新增 SetAccel 命令。
提高：实现带优先级的两个队列合并调度。
思考：设计“命令去重”避免同类频繁重复排队。

## 19. 快速回顾
- 封装操作解耦调用与执行
- 支持排队、记录、撤销
- 嵌入式需控制结构尺寸与队列满策略
