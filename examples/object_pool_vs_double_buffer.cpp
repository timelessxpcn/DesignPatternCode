#include <cstdint>
#include <cstring>
#include <array>

// 对象池与双缓冲差异示例
// 场景：采样数据帧处理 + 命令/事件节点

// -------- 对象池：复用离散小对象 (事件节点) ----------
struct Event {
    uint32_t ts;
    uint8_t  type;
    uint8_t  payload[8];
};

template<size_t N>
class EventPool {
public:
    Event* acquire(){
        if (head_ == N) return nullptr;
        size_t idx = head_;
        head_ = next_[idx];
        Event* e = new(&storage_[idx]) Event{};
        return e;
    }
    void release(Event* e){
        size_t idx = static_cast<Event*>(e) - reinterpret_cast<Event*>(storage_);
        e->~Event();
        next_[idx] = head_;
        head_ = idx;
    }
private:
    alignas(Event) unsigned char storage_[N][sizeof(Event)];
    size_t next_[N];
    size_t head_ = 0;
public:
    EventPool(){
        for(size_t i=0;i<N-1;++i) next_[i]=i+1;
        next_[N-1]=N;
    }
};

// -------- 双缓冲：交替整体缓冲帧 (采样批数据) ----------
template<size_t N>
class DoubleBuffer {
public:
    uint16_t* writeBuf(){ return buf_[writeIdx_].data(); }
    uint16_t* readBuf() { return buf_[readIdx_].data(); }
    void commit() {
        // 交换指针索引
        readIdx_ ^= 1;
        writeIdx_ ^= 1;
    }
private:
    std::array<uint16_t,N> buf_[2]{};
    int writeIdx_ = 0;
    int readIdx_  = 1;
};

// 使用差异示例
// - 对象池：每个事件单独生命周期，离散借出/归还
// - 双缓冲：批量数据整体交换，无单元素 release
void demo(){
    EventPool<16> pool;
    Event* e = pool.acquire();
    if(e){
        e->type = 1;
        pool.release(e);
    }

    DoubleBuffer<128> db;
    // 采样写入
    db.writeBuf()[0] = 123;
    db.commit(); // 读写角色交换
    auto v = db.readBuf()[0]; (void)v;
}

/*
差异摘要：
对象池：
  - 目的：控制大量短生命周期对象的分配释放
  - 操作粒度：单对象 acquire/release
  - 结构：自由链/位图
  - 优点：固定上限、O(1) 分配
  - 缺点：耗尽处理复杂
双缓冲：
  - 目的：生产-消费阶段解耦 & 避免在读写同时修改
  - 操作粒度：整体缓冲区 swap
  - 结构：两个等大缓冲
  - 优点：交换 O(1)，避免拷贝
  - 缺点：仅限 2 套（或扩展多缓冲），占用两倍内存
*/
