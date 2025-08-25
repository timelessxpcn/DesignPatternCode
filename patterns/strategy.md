# 策略模式 (Strategy Pattern)

## 1. 意图
在不修改调用方的情况下可互换一族算法/行为；支持运行期或编译期替换。

## 2. 动机
嵌入式设备需支持多种数据编码或功耗决策算法；初版使用 if-else 逐渐膨胀，修改风险升高。策略将变化点外置，核心循环保持稳定。

## 3. 适用场景
- 多种功耗/休眠决策算法（性能 vs 能耗）
- 编码/压缩策略（不同带宽场景）
- 传感器数据格式化输出（CSV/JSON/二进制）
- 游戏/控制算法选择（PID / 前馈 / 自适应）

## 4. 结构概述
Context 持有抽象策略接口指针或模板参数；调用 export/process 方法而不关心具体实现。

## 5. 基础实现（虚函数版）
```cpp
#include <memory>
#include <string>

struct EncodeStrategy {
    virtual ~EncodeStrategy() = default;
    virtual std::string encode(const std::string& in) = 0;
};

struct PlainText : EncodeStrategy {
    std::string encode(const std::string& in) override { return in; }
};

struct HexUpper : EncodeStrategy {
    std::string encode(const std::string& in) override {
        static const char* digits = "0123456789ABCDEF";
        std::string out;
        out.reserve(in.size() * 2);
        for (unsigned char c : in) {
            out.push_back(digits[c >> 4]);
            out.push_back(digits[c & 0x0F]);
        }
        return out;
    }
};

class EncoderContext {
public:
    explicit EncoderContext(std::unique_ptr<EncodeStrategy> s)
        : strat_(std::move(s)) {}
    void setStrategy(std::unique_ptr<EncodeStrategy> s) { strat_ = std::move(s); }
    std::string apply(const std::string& in) { return strat_->encode(in); }
private:
    std::unique_ptr<EncodeStrategy> strat_;
};
```

## 6. 嵌入式适配要点
- 动态分配：可换成静态对象指针或引用表。
- 虚调用：在紧循环中可能影响分支预测；可用模板静态多态消除。
- 中断上下文：不建议在 ISR 中调用（字符串操作 / 非确定性）。
- 内存：策略对象尽量无状态或轻量，便于放在只读段。
- 无异常环境：不依赖异常，安全。

## 7. 进阶实现
### 7.1 静态多态（模板参数）
```cpp
template <typename Strategy>
class EncoderT {
public:
    explicit EncoderT(Strategy s = {}) : s_(s) {}
    std::string apply(const std::string& in) { return s_.encode(in); }
private:
    Strategy s_;
};
struct HexUpperS {
    std::string encode(const std::string& in) {
        // 同前略
        std::string out; out.reserve(in.size()*2);
        static const char* digits="0123456789ABCDEF";
        for (unsigned char c: in){ out.push_back(digits[c>>4]); out.push_back(digits[c&0x0F]); }
        return out;
    }
};
```
优点：无虚表/堆；缺点：每策略组合生成新实例代码，可能增加 flash。

### 7.2 无堆运行时选择（函数指针表）
```cpp
struct EncodeVTable {
    std::string (*encode)(const std::string&);
};
std::string plainImpl(const std::string& s){ return s; }
// ... hexImpl(略)
class EncoderLight {
public:
    explicit EncoderLight(const EncodeVTable* vt): vt_(vt){}
    std::string apply(const std::string& in){ return vt_->encode(in); }
private:
    const EncodeVTable* vt_;
};
```
减少多态开销控制权；适合少量策略。

## 8. 资源与实时性评估
- 虚函数版：一次虚表间接 + 可能堆分配（创建阶段），执行时间取决于具体策略。
- 静态多态：最优指令路径；风险是代码体积膨胀。
- 函数指针表：单次间接，表可放 ROM。

## 9. 替代比较
- 仅 2 种简单行为：if/switch 更直接。
- C++17 variant + visit：封闭集合且编译期确定策略集合。
- 模板参数：编译期绑定，无运行期切换需求时更好。

## 10. 可测试性
- 运行时策略：可注入 mock。
- 静态多态：需对每个策略类单独测试；可用相同 test fixture 模板化。

## 11. 适用性检查清单
使用条件：
- 算法族 >=3 且可能扩展
- 需要运行期切换 或 需对外暴露统一接口
不使用：
- 行为完全稳定
- 严格禁止间接调用（硬实时内层周期），可改用内联模板

## 12. 常见误用 & 修正
误用：为 2 个简单 if 引入复杂虚类层 → 体积增 & 可读性下降。  
修正：保留简单分支 或 用策略后合并共性数据，防止类爆炸。

## 13. 组合
- 与工厂：根据设备配置字节初始化策略
- 与对象池：频繁切换实例但成本可控
- 与装饰器：编码结果再压缩/加密

## 14. 案例
功耗模式：Performance / Balanced / Eco 三种频率/睡眠阈值策略 → 主循环周期内调用策略决定下次睡眠时长。

## 15. 实验建议
对比三实现（虚函数 / 模板 / 函数指针）在：  
- O2 编译下执行 1e6 次 16 字节编码的耗时  
- size 工具统计 .text 尺寸

## 16. MISRA / AUTOSAR 注意
- 虚函数/多态可以但需文档化用途
- 避免动态分配（若使用需封装与生命周期声明）

## 17. 面试 / Review 问题
- 怎样在无堆环境实现策略可切换？
- 模板多态导致代码尺寸膨胀如何衡量？
- variant 与 虚函数策略优劣？

## 18. 练习
基础：添加 Base64Encode 策略（注意避免堆）。  
提高：实现策略链（多步编码）不使用动态分配。  
思考：在硬实时 1ms 周期任务中使用策略可能的抖动来源？

## 19. 快速回顾
- 解耦一族算法
- 嵌入式需考虑堆/虚调度成本
- 可用模板、函数指针、variant 替代
