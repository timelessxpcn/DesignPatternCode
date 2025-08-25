# 代理模式 (Proxy Pattern)

## 1. 意图
为目标对象提供一个替身以控制访问：增加缓存、延迟、权限、安全、远程通信等。

## 2. 问题背景 / 动机
直接对硬件寄存器或外设操作成本高：可能阻塞或需互斥；或远程外设（通过总线）。通过代理：
- 缓存最近数据减少访问
- 限制访问频率（节能）
- 权限控制（只允许某模块写）
- 远程调用打包（RPC/串口协议）隐藏细节

## 3. 适用场景
- 传感器读缓存（避免频繁 I2C 读取）
- 受保护资源（写需授权）
- 延迟加载（首次访问才初始化）
- 远程固件升级命令封装

## 4. 结构概述
Subject 接口；RealSubject 实际实现；Proxy 持有 RealSubject 指针或访问句柄，在调用前后附加逻辑。嵌入式可无需继承：相同方法签名包装转发。

## 5. 基础实现（缓存代理）
```cpp
struct ISensor { virtual ~ISensor()=default; virtual int read()=0; };

class RealSensor : public ISensor {
public: int read() override { /* I2C 操作 */ return lastRaw(); }
private: int lastRaw(){ return 100; }
};

class CachedSensorProxy : public ISensor {
public:
    explicit CachedSensorProxy(ISensor& real, uint32_t intervalMs)
        : real_(real), interval_(intervalMs) {}
    int read() override {
        uint32_t now = tick();
        if(now - lastUpdate_ > interval_){
            cache_ = real_.read();
            lastUpdate_ = now;
        }
        return cache_;
    }
private:
    ISensor& real_;
    uint32_t interval_;
    uint32_t lastUpdate_{0};
    int cache_{0};
};
```

## 6. 嵌入式适配要点
- 时间源 tick() 必须可用且单调。
- 访问控制：在代理中加入验证（调试期断言或权限标志）。
- 避免把代理堆叠成深链（性能）。
- 若使用延迟初始化，需定义失败回退策略。

## 7. 进阶实现
### 7.1 权限代理
传入授权 token；无权限拒绝（返回错误码，不用异常）。
### 7.2 虚拟代理（延迟初始化）
```cpp
class LazySensorProxy : public ISensor {
public:
    int read() override {
        if(!real_) init();
        return real_->read();
    }
private:
    void init(){ static RealSensor rs; real_=&rs; }
    ISensor* real_{nullptr};
};
```
### 7.3 远程代理 (序列化请求)
封装 `read()` -> 打包帧 -> 串口发送 -> 等待响应。

## 8. 资源与实时性评估
- 额外间接调用 + 缓存判断分支。
- 远程代理：延迟高、需超时处理。
- 内存：少量状态（缓存与时间戳）。

## 9. 替代比较
- 装饰器：附加功能但不必控制访问或懒加载。
- Adapter：改变接口，不是控制访问。
- Facade：简化多个接口，不控制单对象行为。

## 10. 可测试性与可维护性
- 注入伪 RealSubject 模拟慢设备。
- 模拟 tick 推进测试缓存过期。
- 远程代理：模拟通信层注入故障帧测试重试。

## 11. 适用性检查清单
Use：
- 需要缓存/权限/懒加载/远程封装
Avoid：
- 仅添加简单日志（装饰器更轻量）
- 访问频率极高且代理逻辑不可内联

## 12. 常见误用 & 修正
误用：代理里混入大量业务逻辑变为“次系统”。  
修正：代理只处理访问控制相关职责。

误用：缓存代理未处理一致性（底层值改变未察觉）。  
修正：增加失效策略或订阅底层变化。

## 13. 与其它模式组合
- 单例：全局硬件代理
- 装饰器：代理后再叠加日志/指标
- 观察者：底层变化通知代理刷新缓存

## 14. 案例
电池电量读取代理：每 1s 实际读取一次，其余返回缓存，减少 I2C 总线占用。

## 15. 实验建议
- 测量直接读取 vs 代理读取频繁调用耗时
- 远程代理超时行为统计（成功率/平均延迟）

## 16. MISRA / AUTOSAR 注意
- 指针有效性检查
- 超时/失败路径必须明确返回码

## 17. 面试 / Review 问题
- 代理 vs 装饰器区别？
- 远程代理的超时 & 重试策略如何设计？
- 如何避免缓存失效问题？

## 18. 练习
基础：实现 Lazy 初始化代理。
提高：实现权限代理（允许写与只读角色）。
思考：缓存代理如何与中断里更新的实时值同步？

## 19. 快速回顾
- 控制访问（缓存/权限/远程/懒加载）
- 嵌入式关注延迟与一致性
- 避免代理职责膨胀
