# 对象池模式 (Object Pool Pattern)

## 1. 意图
通过预先分配并循环复用一组对象实例，降低运行期动态分配/释放开销与碎片化风险。

## 2. 问题背景 / 动机
嵌入式系统常禁用频繁 new/delete（碎片、不可预测延迟）。例如频繁创建“消息/命令”结构的场景，可用对象池预分配固定数量缓冲，避免实时抖动。

## 3. 适用场景
- 高频短生命周期小对象（消息、命令、事件节点）
- 限制最大并发数量（硬件队列深度上限）
- 禁止运行期动态分配 / 需界定内存上限
- 需要统计利用率以调优容量

## 4. 结构概述
Pool 管理固定数组 + 空闲列表（索引 / 位图 / 单链），客户端向池借出对象句柄（指针或包装器），归还时放回空闲。可选零化/重置。

## 5. 基础实现（单线程简单池）
```cpp
template<typename T, size_t N>
class ObjectPool {
public:
    T* acquire() {
        if (freeHead_ == npos) return nullptr;
        size_t idx = freeHead_;
        freeHead_ = next_[idx];
        T* obj = new(&storage_[idx]) T(); // 定位 new
        return obj;
    }
    void release(T* ptr) {
        size_t idx = static_cast<size_t>(ptr - reinterpret_cast<T*>(storage_));
        ptr->~T();
        next_[idx] = freeHead_;
        freeHead_ = idx;
    }
private:
    static constexpr size_t npos = static_cast<size_t>(-1);
    alignas(T) unsigned char storage_[N][sizeof(T)];
    size_t next_[N];
    size_t freeHead_ = 0;
public:
    ObjectPool() {
        for (size_t i=0;i<N-1;++i) next_[i]=i+1;
        next_[N-1]=npos;
    }
};
```

## 6. 嵌入式适配要点
- 避免 malloc：使用静态数组。
- 需 O(1) acquire/release。
- 如多线程/ISR 访问：需锁或无锁结构（禁用阻塞时可用环形索引 + 原子）。
- 可能需在调试模式加入溢出检测（魔数 / in-use 标志）。

## 7. 进阶实现
### 7.1 位图池（紧凑管理）
适合对象固定小：使用单个 uint32_t 位图表示使用状态，扫描或利用 find-first-clear 指令（若支持）。
### 7.2 无锁（单生产单消费者）与回收队列
使用两个无锁环形缓冲：free_queue / used_queue（按需）。

### 7.3 RAII 句柄包装
```cpp
template<typename T, size_t N>
class PoolHandle {
public:
    PoolHandle(ObjectPool<T,N>* pool, T* obj): pool_(pool), obj_(obj){}
    ~PoolHandle(){ if(obj_) pool_->release(obj_); }
    T* operator->(){ return obj_; }
    // 禁止拷贝，允许移动
private:
    ObjectPool<T,N>* pool_;
    T* obj_;
};
```

## 8. 资源与实时性评估
- 时间：O(1)（数组索引链）。
- 空间：N * sizeof(T) + 额外索引数组。
- 碎片：无（连续存储）。
- 上限：容量固定；需要策略处理耗尽（丢弃 / 覆盖 / 等待）。

## 9. 替代比较
- 动态分配：更灵活但不可预测。
- 静态全局数组：无释放，但浪费未使用槽。
- 内存池（通用分配器） vs 对象池：后者结构简单，对特定 T 优化。

## 10. 可测试性与可维护性
- 测试耗尽路径：连续 acquire N 次。
- 测试重入：release 后再次 acquire。
- 统计最大并发使用峰值，用于调优 N。

## 11. 适用性检查清单
Use：
- 高频创建释放
- 固定上限可预估
Avoid：
- 对象数高度动态且潜在远超固定上限
- 对象构造复杂需不同参数（难以统一复用）

## 12. 常见误用 & 修正
误用：为大对象（>几 KB）大量预分配造成内存浪费。  
修正：改用按需分层池或延迟（slab 分级）。

## 13. 组合
- 与工厂：工厂从池获取对象而非 new。
- 与命令模式：命令节点池化。
- 与双缓冲：池内对象作为缓冲帧元素。

## 14. 案例
事件队列节点（含时间戳与 payload 指针）放在对象池，ISR 中仅 acquire + 填写，主循环处理后 release。

## 15. 实验建议
测量：acquire/release 循环 1e6 次 vs new/delete；统计指令周期与 .text 大小差异。

## 16. MISRA / AUTOSAR 注意
- 使用定位 new 需审查对象生命周期与异常（若禁异常问题小）。
- 确保不返回已释放对象悬垂指针。

## 17. 面试 / Review 问题
- 耗尽策略有哪些？各自风险？
- 如何在多线程/ISR 下保持 O(1)？
- 何时池化反而增加复杂度？

## 18. 练习
基础：添加 inUse 标志检查重复释放。
提高：实现位图池 acquire（返回首个空位）。
思考：设计耗尽时降级策略（例如：返回静态备用对象）。

## 19. 快速回顾
- 预分配复用降低抖动
- 固定容量与耗尽策略是关键
- 适合小而频繁对象
