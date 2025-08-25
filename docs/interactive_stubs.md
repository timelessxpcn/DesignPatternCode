# 交互平台题目 Stubs (首批 10 题)

格式约定：
- ID
- 标题
- 类型: 实现 / 重构 / 选型 / 修复 / 性能
- 描述（嵌入式背景）
- 要求
- 输入输出 (若适用)
- 约束 (禁止动态分配 / 时间等)
- 验证点
- 自动测试思路
- 静态分析规则
- 扩展/加分

---

## 1. ID: P001
标题：策略替换功耗算法
类型：实现
描述：已有 Context 调用 `calc_sleep_ms(load)`，请实现两个策略 Performance / Eco。
要求：Performance 负载低返回 5ms，Eco 低返回 20ms。
约束：禁止 new/delete。
验证：切换策略后结果不同。
测试：构造 load=10/80。
静态分析：禁止出现 `new`.
加分：模板静态策略版本。

## 2. ID: P002
标题：对象池实现事件节点
类型：实现
描述：实现固定大小 32 节点事件池，支持 acquire/release。
要求：O(1)；耗尽返回 nullptr。
约束：不使用 malloc。
测试：acquire 32 次全非空，第 33 次空；释放后可重用。
静态分析：搜索 `malloc|new`.
加分：记录峰值利用率。

## 3. ID: P003
标题：状态机驱动功耗管理
类型：实现
描述：设备状态 Boot->Init->Run->LowPower。
要求：按 tick 驱动转换逻辑。
输入：模拟 tick 迭代次数。
输出：最终状态枚举。
测试：给定 50 tick 期望进入 LowPower 至少一次。
静态分析：无动态分配。
加分：表驱动版本。

## 4. ID: P004
标题：适配遗留温度 API
类型：重构
描述：legacy 函数返回温度 *10，需适配到 `ISensor::readCelsius()`.
要求：添加 Adapter 类。
测试：模拟 legacy 返回 253 → 25.3f。
静态分析：禁止修改 legacy.h。
加分：缓存刷新间隔实现。

## 5. ID: P005
标题：双缓冲采样处理
类型：实现
描述：实现 write buffer / read buffer 交换，采样写入一侧，处理读取另一侧。
要求：swap O(1)，无堆。
测试：100 次循环无数据覆盖错误。
静态分析：无 new/delete。
加分：加入原子标志支持 ISR 生产。

## 6. ID: P006
标题：对象池 vs 动态分配性能对比
类型：性能
描述：比较 10 万次获取/释放节点耗时。
要求：输出两个耗时 (微秒)。
测试：阈值判断对象池更快（或更稳定）。
静态分析：必须存在对象池实现。
加分：记录标准差。

## 7. ID: P007
标题：工厂创建不同传感器
类型：实现
描述：实现 `createSensor(id)` 返回静态实例 A/B。
约束：无堆。
测试：id=0/1 返回非空且指针不同。
静态分析：无 new。
加分：constexpr 映射表。

## 8. ID: P008
标题：命令对象池复用
类型：重构
描述：现有代码每次 new 命令对象；重构用对象池。
要求：接口不变。
测试：执行 100 次命令无内存泄漏 (池利用率)。
静态分析：禁止 new Command。
加分：RAII 句柄。

## 9. ID: P009
标题：选择模式说明（Strategy vs State）
类型：选型
描述：给出两个场景（算法可替换 / 设备模式转换）写明各自使用的模式与理由。
要求：文字 ≤ 300 字。
测试：关键字检验包含 “策略” “状态” “原因”。
静态：无代码限制。
加分：列出误用后果。

## 10. ID: P010
标题：环形缓冲生产者消费者
类型：实现
描述：单生产单消费环形队列（容量 16）。
要求：push/pop O(1)，满/空区分。
测试：写入 16 元素，第 17 次失败；读回顺序正确。
静态分析：无 new。
加分：模板泛型化。

---

# 自动验证总体思路

1. 单元测试：针对实现型题目编写 gtest/catch2 测试；选型题解析用正则检查关键术语。
2. 静态分析：
   - 动态分配禁用：grep/clang-tidy AST 匹配 `CXXNewExpr`
   - ISR 安全题可检查是否使用互斥（后续扩展）
3. 性能题（P006）：
   - 使用 chrono 记录多次循环平均值
   - 判定对象池版本耗时 < 动态分配 * 阈值（如 0.7）或稳定性（标准差小）
4. 代码风格：
   - clang-format --dry-run
5. 资源使用估计（后续增强）：
   - 编译生成 map / size 报告，解析 .text 增量
6. 结果反馈：
   - 汇总 JSON：{passed, failed_tests, style_violations, time_metrics}

# 示例伪代码（自动测试脚本构想）
```bash
# build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# run tests
ctest --output-on-failure

# static allocation check
if grep -R "new " submissions/P002.cpp; then echo "FAIL new detected"; fi
```

# 后续扩展
- 集成 run-clang-tidy 输出 MISRA 子集
- 失败用例映射回章节提示（题目 ID -> pattern .md）
