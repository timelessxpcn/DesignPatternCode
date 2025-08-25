# 生产者-消费者模式 (Producer-Consumer Pattern)

## 1. 意图
通过一个共享缓冲（队列 / 环形缓冲）解耦数据生产与消费速率，实现缓冲平滑、降低耦合和控制背压。

## 2. 问题背景 / 动机
采样中断产生数据快（微秒级），主循环或任务处理较慢（毫秒级）。直接在 ISR 里执行完整处理会突破实时预算。引入环形缓冲：ISR 仅快速写入，主循环慢速读取。

## 3. 适用场景
- 传感器/ADC 采样 -> 处理/滤波
- UART/串口接收 ISR -> 协议解析
- 日志写入缓冲 -> 后台异步输出
- 网络数据包接收 -> 任务级解包

## 4. 结构概述
共享缓冲、读指针（consumer）、写指针（producer）、判空/判满策略。单生产单消费可用无锁环形；多生产或多消费需加锁或使用原子序逻辑。

## 5. 基础实现（单生产单消费无锁环形缓冲）
```cpp
template<typename T, size_t N>
class RingBuffer {
public:
    bool push(const T& v){
        size_t next = (write_ + 1) % N;
        if (next == read_) return false; // full
        buf_[write_] = v;
        write_ = next;
        return true;
    }
    bool pop(T& out){
        if (read_ == write_) return false; // empty
        out = buf_[read_];
        read_ = (read_ + 1) % N;
        return true;
    }
    bool empty() const { return read_ == write_; }
    bool full()  const { return ( (write_+1)%N ) == read_; }
private:
    T buf_[N];
    size_t write_{0};
    size_t read_{0};
};
static_assert((16 & (16-1))!=0, "N not necessarily power of two (mod used)"); // 示例
```

## 6. 嵌入式适配要点
- ISR 写入需 O(1) 且最少分支。
- 如果 N 为 2^k，可用位与加速：`next = (write_ + 1) & (N-1)`.
- 多任务（RTOS）场景：加互斥或使用轻量信号量；单生产单消费无需锁。
- 需要背压策略：满时覆盖旧数据 / 丢弃新数据 / 计数丢包。

## 7. 进阶实现
### 7.1 覆盖策略（丢最旧）
```cpp
bool pushOverwrite(const T& v){
    size_t next = (write_+1)%N;
    if(next == read_) { // full -> advance read (drop oldest)
        read_ = (read_+1)%N;
    }
    buf_[write_] = v;
    write_ = next;
    return true;
}
```
### 7.2 多生产多消费（简化锁）
使用轻量自旋锁或 `std::atomic<size_t>` 进行比较交换推进指针（若可用）。

### 7.3 批量操作
减少指针操作开销：批量写 N 个样本再一次更新写指针。

## 8. 资源与实时性评估
- 内存：N * sizeof(T)
- 时间：push/pop O(1)
- 抖动：与冲突少、无分配，确定性高

## 9. 替代比较
- 使用队列 + 动态分配：更灵活但不确定。
- 双缓冲：适合固定大小批次；环形适合连续流。
- 对象池 + 链表：当元素大小各异或需复用节点。

## 10. 可测试性与可维护性
- 测试满、空、覆盖逻辑。
- 压力测试：循环 push/pop 计数验证顺序。
- 模拟 ISR 环境：在测试线程高频 push，主线程 pop。

## 11. 适用性检查清单
Use：
- 连续流数据与速率差
- 需 O(1) 缓冲操作
Avoid：
- 高度突发 + 容量远超静态预算（可能需动态扩展结构）
- 数据批处理固定边界 → 可用双缓冲简化

## 12. 常见误用 & 修正
误用：多生产者同时使用无锁 SPSC 环形导致数据竞争。  
修正：限制为 SPSC 或升级为 MPMC 结构（原子 CAS）。

误用：忽略满队列错误码（数据悄然丢失）。  
修正：增加统计计数或覆盖策略显式记录。

## 13. 与其它模式组合
- 与对象池：队列存放指向池对象指针。
- 与观察者：消费者处理后触发通知。
- 与命令模式：命令对象排队。

## 14. 案例
UART ISR 每次接收字节 push 到环形缓冲；主循环解析协议帧。满时统计丢字节计数用于诊断。

## 15. 实验建议
- 比较 N=32/128/512 的缓存命中与丢包率。
- push/pop 周期，在 O2 下汇编检查是否内联优化。

## 16. MISRA / AUTOSAR 注意
- 对共享变量的访问需保证原子性（单核 + 禁止中断窗口）。
- 避免数据竞争：ISR 与主循环共用指针应 `volatile` 或记入内存屏障策略。（如编译器重排风险）

## 17. 面试 / Review 问题
- 如何选择覆盖 vs 丢弃策略？
- 双缓冲与环形缓冲在采样场景差异？
- 如何支持多生产者？

## 18. 练习
基础：实现覆盖策略 push。
提高：实现 power-of-two 优化索引版本。
思考：估算合适容量：给定生产 2kHz、消费 1.5kHz。

## 19. 快速回顾
- 解耦速率差异
- SPSC 环形最轻量
- 背压与丢包策略要显式
