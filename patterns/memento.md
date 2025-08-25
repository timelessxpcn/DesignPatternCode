# 备忘录模式 (Memento Pattern)

## 1. 意图
在不破坏封装的前提下捕获对象内部状态并在以后恢复，支持撤销/回滚/故障恢复。

## 2. 问题背景 / 动机
嵌入式需要在参数调整、固件升级、设备模式切换失败时回滚；如果直接暴露内部结构修改风险高。备忘录将关键状态快照保存（RAM/Flash），必要时恢复。

## 3. 适用场景
- 参数调优临时更改 -> 可撤销
- 配置写 EEPROM 前先保存旧状态
- 升级失败回滚运行参数/校准数据
- 状态机模式回退（少数关键变量）

## 4. 结构概述
Originator 负责创建/恢复 Memento；Memento 保存内部必要状态（通常为私有结构）；Caretaker 管理 Memento 生命周期（栈/列表）。嵌入式中需关注存储空间与写耐久。

## 5. 基础实现（内存快照）
```cpp
struct DeviceConfig {
    uint16_t speed;
    uint8_t mode;
    uint32_t flags;
};

class ConfigOriginator {
public:
    void apply(const DeviceConfig& c){ cfg_ = c; }
    struct Memento { DeviceConfig snapshot; };
    Memento save() const { return Memento{cfg_}; }
    void restore(const Memento& m){ cfg_ = m.snapshot; }
    const DeviceConfig& current() const { return cfg_; }
private:
    DeviceConfig cfg_{};
};
```

## 6. 嵌入式适配要点
- Snapshot 放栈/静态时注意大小（大数据应压缩或引用）。
- Flash/EERPOM 回滚：写次数限制 → 频繁撤销不适合持久化。
- 可能采用环形日志 (只写追加) 实现多版本历史。
- 敏感数据（密钥）避免明文备份（加密/清除）。

## 7. 进阶实现
### 7.1 增量差异 (Delta)
存储与前一个快照差异，减少空间。
### 7.2 压缩/校验
Memento 加 CRC 校验恢复前验证。
### 7.3 多层历史栈
固定深度数组循环覆盖（最近 N 步撤销）。

## 8. 资源与实时性评估
- 拷贝成本 = 状态大小
- 恢复 O(n)
- Flash 写需考虑延迟与磨损，必要时异步。

## 9. 替代比较
- 直接重新计算状态：如果重建成本低则无需快照。
- Command + Undo：针对操作的逆操作，不存全状态。
- 版本号 + 配置结构：简单回滚到稳定版本（少量）。

## 10. 可测试性与可维护性
- 测试保存→修改→恢复一致性。
- 测试多级恢复（LIFO）。
- 测试 CRC 破坏时拒绝恢复。

## 11. 适用性检查清单
Use：
- 状态重建成本高
- 需多级撤销或故障回滚
Avoid：
- 状态小且重建便宜
- 写耐久限制严苛不允许频繁快照

## 12. 常见误用 & 修正
误用：快照过大（包含无需字段）。  
修正：只存最小必要集合或差异。

误用：频繁写 Flash 制造磨损。  
修正：RAM 历史 + 间隔持久化。

## 13. 与其它模式组合
- 命令：命令执行前保存 Memento
- 状态机：进入高风险状态前快照
- 备忘录 + 原型：用原型快速复制做快照

## 14. 案例
参数调优 UI：每次应用前保存 Memento；用户取消时 restore 回滚。

## 15. 实验建议
- 测量保存/恢复时间与大小。
- Delta 模式空间节省百分比。

## 16. MISRA / AUTOSAR 注意
- 结构对齐，避免 memcpy 未定义行为。
- Flash 写需错误码处理。

## 17. 面试 / Review 问题
- 与 Undo 命令差异？
- Delta 方案何时优于全量？
- 如何处理 Flash 寿命？

## 18. 练习
基础：实现固定深度历史栈。
提高：实现 Delta（只存变化字段）。
思考：设计带 CRC 的持久快照恢复流程。

## 19. 快速回顾
- 保存内部状态快照以回滚
- 成本=状态大小+写耐久
- Delta/压缩/CRC 提升效率与可靠性
