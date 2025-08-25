# 双缓冲模式 (Double Buffer / Ping-Pong Buffer)

## 1. 意图
使用两个等大小缓冲交替（一个写入，一个读取/处理），实现生产与消费阶段分离，减少同步开销与数据竞争。

## 2. 问题背景 / 动机
ADC 或 DMA 持续写入采样数据，处理算法（滤波 / FFT）需要整块数据。若直接在写入中读取会遇到部分更新/一致性问题；锁增加延迟。双缓冲允许一次填满写缓冲后快速交换指针，处理稳定数据。

## 3. 适用场景
- DMA 采样 + 信号处理
- 显示帧缓冲（后台绘制 + 前台显示）
- 网络收包分段批处理
- 日志批量刷新

## 4. 结构概述
两个缓冲数组 A/B；标志（或索引）指示哪个是“写缓冲”哪个是“读缓冲”。生产者完成写入 -> 调用 `swap()` 或设置交换标志；消费者在下一周期处理旧数据。

## 5. 基础实现（无锁 SPSC）
```cpp
template<typename T, size_t N>
class DoubleBuffer {
public:
    T* writeBuf() { return buf_[write_]; }
    const T* readBuf() { return buf_[read_]; }

    // 由生产者调用
    void commit() {
        // 交换索引
        read_ ^= 1;
        write_ ^= 1;
    }
private:
    T buf_[2][N]{};
    int write_{0};
    int read_{1};
};
```

## 6. 嵌入式适配要点
- DMA：配置 DMA 写入 `writeBuf()`；中断完成后 `commit()`.
- ISR 与主循环：ISR 只做 `commit()`（O(1)），主循环处理 `readBuf()`.
- 必须确保在 commit 前消费者不访问写缓冲。
- 数据大小大时注意总内存（2x N）。
- 若需要多阶段流水线：可扩展为多缓冲（环形 K>2）。

## 7. 进阶实现
### 7.1 原子交换标志（多核或任务）
```cpp
#include <atomic>
template<typename T, size_t N>
class DoubleBufferAtomic {
public:
    T* writeBuf(){ return buf_[write_.load(std::memory_order_relaxed)]; }
    const T* readBuf(){ return buf_[read_.load(std::memory_order_acquire)]; }
    void commit() {
        int w = write_.load(std::memory_order_relaxed);
        int r = read_.load(std::memory_order_relaxed);
        read_.store(w, std::memory_order_release);
        write_.store(r, std::memory_order_relaxed);
    }
private:
    T buf_[2][N]{};
    std::atomic<int> write_{0};
    std::atomic<int> read_{1};
};
```
### 7.2 帧序号与丢帧检测
在交换时递增 `frameCounter`，消费者检查是否跳变 >1 判断丢帧。

### 7.3 多缓冲（环形 K 缓冲）
当处理时间接近采样间隔导致潜在滞后，增加缓冲深度缓冲抖动。

## 8. 资源与实时性评估
- 内存：2x N * sizeof(T)
- 交换：O(1)，极低 jitter
- 无锁：生产/消费基本独立
- 不适合大小高度不规则数据（需对象池）

## 9. 替代比较
- 环形缓冲：适合连续流；双缓冲适合批处理整块
- 对象池：更灵活但管理复杂
- 单缓冲 + 锁：简单但增加阻塞和数据不一致风险

## 10. 可测试性与可维护性
- 测试交替正确：写入显式模式数据（递增），commit 后消费者读取不被覆盖。
- 测试处理耗时 > 生产间隔情形（模拟丢帧）。
- 注入 mock DMA 完成回调。

## 11. 适用性检查清单
Use：
- 固定长度批数据
- 高速生产 + 需要一致快照
Avoid：
- 数据大小可变频繁变化
- 内存极紧张（2x 缓冲不可接受）

## 12. 常见误用 & 修正
误用：消费者在 commit 前读取写缓冲。  
修正：API 明确 read/write 指针，不直接暴露原始数组或加入 `processingFlag`。

误用：处理时间过长导致隐性丢帧未检测。  
修正：加入帧计数 + 日志。

## 13. 与其它模式组合
- 与对象池：双缓冲指向池中两组对象集合
- 与生产者消费者：双缓冲作为底层结构简化队列锁
- 与策略：每帧处理算法策略可切换

## 14. 案例
麦克风音频 256 样本一帧：DMA 填充写缓冲 -> 中断 commit -> 主循环对 read 缓冲做 FFT。

## 15. 实验建议
- 测量 commit 与 memcpy 复制 256 样本的耗时差（交换应显著小）。
- 模拟处理时间百分比 > 80% 时帧丢失率。

## 16. MISRA / AUTOSAR 注意
- 原子或屏蔽中断保护交换 (单核)。
- 禁止越界访问 (编译期 N 常量)。

## 17. 面试 / Review 问题
- 双缓冲 vs 环形缓冲选择标准？
- 如何检测并处理消费者滞后？
- 扩展到三缓冲的意义与代价？

## 18. 练习
基础：实现帧计数 + 丢帧检测。
提高：扩展为 K=3 ring double buffer。
思考：当处理时间不可预测时策略（降采样 / 丢帧 / 延时处理）。

## 19. 快速回顾
- 两块缓冲交替保证数据一致快照
- O(1) 交换，适合批处理
- 内存翻倍是主要权衡
