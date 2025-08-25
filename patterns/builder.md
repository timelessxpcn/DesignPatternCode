# 建造者模式 (Builder Pattern)

## 1. 意图
将一个复杂对象的构建步骤与其最终表示分离，使同样的构建过程可创建不同表示，并将“逐步构建/可选参数”封装，避免臃肿构造函数与易错的多重初始化顺序。

## 2. 问题背景 / 动机
在嵌入式中，一个通信帧 / 固件升级包 / 传感器复合配置对象可能包含一组可选字段（校验、时间戳、扩展标志），直接使用长参数列表或多重重载构造导致：
- 可读性差 (长参数顺序容易出错)
- 可选字段缺省逻辑分散
- 条件编译下字段增减引起调用处连锁修改  
Builder 允许分阶段设置字段，最后一次性构建，集中校验与默认值填充，并可在不支持异常的环境中用返回码报告失败。

## 3. 适用场景
- 复杂数据帧（协议字段多且部分可选）
- 设备启动配置（时钟、外设、低功耗参数）
- 复合指令批次（多命令打包发送）
- 固件 meta 信息（版本、校验、分区、签名）

## 4. 结构概述
Director（可选）负责调用 Builder 的步骤；Builder 维护中间状态，最终 produce() / build() 返回产品。嵌入式中常省略 Director 直接链式调用。

## 5. 基础实现（链式无异常，返回状态）
```cpp
struct Packet {
    uint8_t type{};
    uint16_t length{};
    uint32_t timestamp{};
    bool hasCrc{false};
    uint16_t crc{};
};

class PacketBuilder {
public:
    PacketBuilder& type(uint8_t t){ pkt_.type = t; return *this; }
    PacketBuilder& length(uint16_t l){ pkt_.length = l; return *this; }
    PacketBuilder& timestamp(uint32_t ts){ pkt_.timestamp = ts; return *this; }
    PacketBuilder& enableCrc(bool en){ pkt_.hasCrc = en; return *this; }
    bool build(Packet& out){
        if(!validate()) return false;
        if(pkt_.hasCrc) pkt_.crc = calcCrc();
        out = pkt_;
        return true;
    }
private:
    bool validate() const {
        return pkt_.length <= 1024 && pkt_.type < 10;
    }
    uint16_t calcCrc() const { return 0xABCD; }
    Packet pkt_{};
};
```

## 6. 嵌入式适配要点
- 避免动态分配：Builder 自身栈或静态使用。
- 无异常：build 返回 bool/错误码。
- 受限内存：产品对象可“就地构建”（传入引用填充）。
- 多配置分支用条件编译减少无关字段。

## 7. 进阶实现
### 7.1 分阶段（Phase）构建
确保某些步骤顺序（比如：必须先设置长度再计算 CRC），可使用不同阶段类型返回：
```cpp
struct StageLength;
struct StageType {
    Packet p;
    StageLength setLength(uint16_t l);
};
struct StageLength {
    Packet p;
    StageLength& enableCrc();
    bool build(Packet& out);
};
```
通过类型系统强制顺序。

### 7.2 轻量 Director（脚本化流程）
```cpp
class PacketDirector {
public:
    static bool makeHeartbeat(Packet& out){
        return PacketBuilder().type(1).length(8).enableCrc(true).build(out);
    }
};
```

### 7.3 条件字段（编译期选择）
```cpp
template<bool EnableCrc>
class PacketBuilderT { /* 若 !EnableCrc 则忽略 CRC 逻辑 */ };
```

## 8. 资源与实时性评估
- 构建开销主要是字段赋值 & 校验 O(n)（字段数）。
- 类型阶段（phase）实现可能增加模板实例代码体积。
- 无持续运行期成本（构建后对象独立）。

## 9. 替代比较
- 直接聚合 struct + 初始化列表：字段少时更简单。
- 可选字段少：多个重载构造可能足够。
- Fluent 设置函数：若缺少统一 build 校验，错误风险增大。

## 10. 可测试性与可维护性
- 独立测试 validate/校验逻辑。
- Director 封装常用预设便于回归。
- 阶段类型强制顺序减少测试用例组合。

## 11. 适用性检查清单
Use：
- 可选/组合字段 > ~4 且需统一校验
- 构建过程需强制或验证顺序
Avoid：
- 简单 POD 几个字段
- 构建必须极端高频且性能敏感（直接 struct 初始化）

## 12. 常见误用 & 修正
误用：字段极少依旧引入复杂 Builder。  
修正：直接聚合初始化。

误用：build 内部做昂贵 I/O（非纯构建）。  
修正：将 I/O 移到单独步骤。

## 13. 与其它模式组合
- 抽象工厂：工厂内部使用 Builder 构造复杂产品族实例。
- 原型：Builder 初始化基于原型拷贝再调整。
- 装饰器：构建完后再包装附加行为。

## 14. 案例
LoRa 帧：设置 type / payload length / 可选 CRC / 时间戳 —> build 集中校验 MTU。

## 15. 实验建议
- 比较多重重载构造 vs Builder 代码行、错误率（模拟错误参数排序）。
- 测量链式 vs 阶段模板闪存尺寸差异。

## 16. MISRA / AUTOSAR 注意
- 链式接口返回 *this 确保未返回悬垂引用。
- 校验失败路径必须显式处理。

## 17. 面试 / Review 问题
- 如何防止遗漏关键字段？
- 与抽象工厂在嵌入式的边界？
- 如何避免 Builder 自身膨胀？

## 18. 练习
基础：为 PacketBuilder 增加 payload 指针设置 + 长度一致性校验。
提高：实现 Phase 编译期顺序强制版本。
思考：在无 RTTI/异常情况下提供详细错误码策略。

## 19. 快速回顾
- 解决复杂/可选字段构建
- 可用 Phase/模板强化顺序与裁剪
- 避免对简单结构过度使用
