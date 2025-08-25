# 组合模式 (Composite Pattern)

## 1. 意图
将对象组织成树形结构使客户端对单个对象与组合对象的使用一致（统一接口）。

## 2. 问题背景 / 动机
嵌入式菜单系统、设备层级组件（系统->子模块->子设备）、配置节点（层级参数）通常需遍历树进行更新/渲染/命令分发。若对叶子和容器区分写大量 if/类型判断，代码可读性差。组合模式提供统一接口操作整个树。

## 3. 适用场景
- LCD/串口菜单树渲染
- 参数配置树（层级读取/更新）
- 多通道设备集合启动/停止遍历
- 复合命令执行（子命令组合）

## 4. 结构概述
Component（统一接口） → Leaf 与 Composite（包含一组子 Component）。嵌入式为减小开销，可：
- 使用数组+索引代替动态 `std::vector`
- 避免频繁添加/删除（树静态构建）
- 使用平铺数组 + parent/firstChild/nextSibling 索引（cache 友好）

## 5. 基础实现（静态节点数组 + 索引）
```cpp
struct Node {
    const char* name;
    int firstChild;   // 索引
    int nextSibling;  // 索引
    bool isLeaf;
    void (*action)();
};

constexpr Node nodes[] = {
    {"ROOT", 1, -1, false, nullptr},
    {"MenuA", -1, 2, true, [](){/*...*/}},
    {"MenuB",  -1, -1, true, [](){/*...*/}}
};

void traverse(int idx){
    int n = idx;
    while(n != -1){
        if(nodes[n].isLeaf && nodes[n].action) nodes[n].action();
        if(!nodes[n].isLeaf && nodes[n].firstChild!=-1)
            traverse(nodes[n].firstChild);
        n = nodes[n].nextSibling;
    }
}
```

## 6. 嵌入式适配要点
- 避免堆与递归（可用显式栈迭代）。
- 字符串常量放 ROM。
- 若节点数大且遍历频繁，平铺数组比指针链更友好（指令预测与缓存）。
- 修改结构不频繁，可在编译期生成。

## 7. 进阶实现
### 7.1 编译期生成树 (constexpr)
使用 `constexpr` 构建节点表 + 生成关系索引。
### 7.2 迭代式遍历
```cpp
void traverseIter(int root){
    int stack[16]; int sp=0;
    int cur=root;
    while(cur!=-1){
        if(nodes[cur].isLeaf && nodes[cur].action) nodes[cur].action();
        if(nodes[cur].firstChild!=-1){
            if(nodes[cur].nextSibling!=-1) stack[sp++]=nodes[cur].nextSibling;
            cur=nodes[cur].firstChild;
        } else if(nodes[cur].nextSibling!=-1){
            cur=nodes[cur].nextSibling;
        } else if(sp){
            cur=stack[--sp];
        } else cur=-1;
    }
}
```
### 7.3 CRTP 接口 (面向对象简化)
只在需要多态扩展少量子类时使用。

## 8. 资源与实时性评估
- 遍历 O(N)。
- 树静态：无增加删除开销。
- 递归风险：有限堆栈（改迭代）。

## 9. 替代比较
- 简单两级结构：直接数组嵌套即可。
- 使用映射访问：若节点按 ID 查找频繁，可建立索引表。
- 若只需批处理而不关心层级：扁平列表更高效。

## 10. 可测试性与可维护性
- 提供验证函数：检查 firstChild/nextSibling 索引合法。
- 单测遍历顺序与预期序列对比。
- 生成脚本生成节点表减少手写错误。

## 11. 适用性检查清单
Use：
- 树形层级操作统一化
- 叶节点与组合需同接口
Avoid：
- 数据结构稳定简单（不必要引入抽象层）
- 对节点类型差异行为极大（策略 + 容器可能更清晰）

## 12. 常见误用 & 修正
误用：每个节点动态分配 + 指针链接 → 内存碎片。  
修正：使用静态数组 + 索引。

误用：叶子行为差异过大仍强制同接口 -> 复杂条件分支。  
修正：拆分类或策略注入。

## 13. 与其它模式组合
- 装饰器：为节点添加附加行为
- 访问者：统一执行不同操作（统计/渲染）
- 命令：组合节点触发一组命令

## 14. 案例
菜单系统：用户输入方向键遍历节点树；叶节点执行回调。

## 15. 实验建议
- 比较递归 vs 迭代遍历堆栈使用
- 节点数扩展时闪存/运行时间曲线

## 16. MISRA / AUTOSAR 注意
- 数组索引边界检查
- 函数指针使用验证非 NULL

## 17. 面试 / Review 问题
- 为什么用索引替代指针？
- 如何检测树结构损坏？
- 与装饰器、访问者如何协作？

## 18. 练习
基础：添加子目录节点并遍历执行。
提高：实现迭代器 next() 逐节点访问。
思考：如何在不使用递归的情况下支持 128 层深度？

## 19. 快速回顾
- 统一对叶/组合处理
- 嵌入式：静态数组 + 索引更高效
- 避免不必要动态分配
