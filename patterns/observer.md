# 观察者模式 (Observer Pattern)

## 1. 意图
在对象间建立一对多依赖，使得当“被观察者”(Subject) 状态变化时自动通知所有“观察者”(Observer)，降低耦合并支持动态订阅/退订。

## 2. 问题背景 / 动机
在嵌入式设备中，多个模块需要对同一事件来源（传感器阈值、通信帧到达、功耗模式变化）做出反应。初版代码可能：事件源直接调用一堆硬编码回调函数或设置全局标志。随着监听方增加，修改事件源风险增大且不可在单元测试中替换。观察者模式抽象出订阅机制，事件源无需了解具体监听者类型。

## 3. 适用场景
- 传感器数据超过阈值 -> 多模块响应（记录日志 / 发送告警 / 调整功耗）
- 通信协议栈：下层帧接收 -> 协议解析 / 统计模块
- 状态机状态变化广播
- 参数配置热更新 (配置模块更新后通知功能模块刷新缓存)

## 4. 结构概述
Subject 保存观察者集合；状态（或事件）变化时迭代通知。嵌入式中可选：
- 静态数组 + 数量计数（无动态分配）
- 位图/索引表
- 单链或环形缓冲 (事件排队异步通知)

## 5. 基础实现（无堆静态数组）
```cpp
#include <cstddef>
#include <cstdint>

struct IObserver {
    virtual ~IObserver() = default;
    virtual void onEvent(uint32_t code, int value) = 0;
};

template<size_t MAX_OBS>
class Subject {
public:
    bool attach(IObserver* obs){
        if (!obs || count_ >= MAX_OBS) return false;
        for (size_t i=0;i<count_;++i) if (observers_[i]==obs) return false;
        observers_[count_++] = obs;
        return true;
    }
    bool detach(IObserver* obs){
        for(size_t i=0;i<count_;++i){
            if(observers_[i]==obs){
                observers_[i]=observers_[count_-1];
                --count_;
                return true;
            }
        }
        return false;
    }
    void notify(uint32_t code, int value){
        for(size_t i=0;i<count_;++i){
            observers_[i]->onEvent(code,value);
        }
    }
private:
    IObserver* observers_[MAX_OBS]{};
    size_t count_{0};
};
```

## 6. 嵌入式适配要点
- 避免动态分配：使用固定数组或编译期容量。
- ISR 中触发 notify：若观察者执行耗时操作，改为记录事件后主循环分发。
- 需处理订阅/退订时机：避免遍历中修改集合导致不一致。
- 回调执行时间不可预测 → 可用事件队列实现异步化。

## 7. 进阶实现
### 7.1 异步事件队列 (对象池 + 生产者消费者)
```cpp
struct Event { uint32_t code; int value; };
template<size_t N>
class EventBus {
public:
    bool push(Event e){
        size_t next = (write_+1)%N;
        if(next==read_) return false; // full
        buf_[write_] = e; write_ = next; return true;
    }
    bool poll(Event& out){
        if(read_==write_) return false;
        out = buf_[read_]; read_=(read_+1)%N; return true;
    }
private:
    Event buf_[N];
    size_t write_{0}, read_{0};
};
```
主循环：poll -> Subject.notify()。ISR：push。

### 7.2 无虚函数版（函数指针 + context）
```cpp
struct ObserverFn {
    void (*fn)(void*, uint32_t, int);
    void* ctx;
};
```
存数组 `ObserverFn`，降低虚表开销。

### 7.3 位图订阅（小集合快速判断）
每个事件类型有一个位图，或同一 Subject 用位号标识已注册槽。

## 8. 资源与实时性评估
- 通知耗时 = 观察者数量 * 单回调开销。
- 固定数组：O(n) 附加；可预测性良好。
- 异步队列：增加一次内存拷贝，但避免长时间阻塞产生 jitter。

## 9. 与语言/库特性替代比较
- 简单 1:1 通知：直接函数调用即可。
- C++20 std::function 列表：更灵活但可能引入堆分配 (small buffer 视实现而定)。
- 信号/槽库（第三方）：功能强但体积增。

## 10. 可测试性与可维护性
- 注入伪观察者记录收到事件序列（断言顺序/次数）。
- 通过 detach 测试移除逻辑。
- EventBus 分离可模拟溢出情形。

## 11. 适用性检查清单
Use：
- 需动态增减监听者
- 一个事件源影响多组件
Avoid：
- 永远只有一个固定监听者
- 回调链过长导致实时不确定（改异步）

## 12. 常见误用 & 修正
误用：在回调中再次修改 Subject 结构（attach/detach），导致迭代失效。  
修正：标记延迟操作；通知后统一处理。

误用：回调执行阻塞 (I/O)。  
修正：队列化 + 主循环处理。

## 13. 与其它模式组合
- 与对象池：事件节点池化
- 与状态机：状态变化 -> 观察者更新 UI / 统计
- 与命令模式：回调中生成命令入队

## 14. 案例
温度阈值模块检测超限 -> 通知日志模块 + 蜂鸣器控制 + 发送远程告警。

## 15. 性能/尺寸实验建议
- 测试 1..N 观察者下平均通知时间。
- 比较虚函数 vs 函数指针实现 .text 尺寸。

## 16. MISRA / AUTOSAR 注意
- 函数指针调用需确保非 NULL。
- 回调执行需文档说明最大时间。

## 17. 面试 / Review 问题
- 如何防止观察者内存已释放仍被调用？
- 回调执行顺序需要保证吗？如何？
- 同一事件高频触发的削峰办法？

## 18. 练习
基础：实现函数指针版观察者。
提高：添加延迟 attach 列表避免迭代冲突。
思考：设计带优先级的通知机制（高优先级先执行）。

## 19. 快速回顾
- 一对多事件传播
- 嵌入式需关注回调时间与 ISR 安全
- 可通过队列实现异步解耦
