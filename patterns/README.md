# Patterns 目录说明 (更新)

已完成模式：
1. strategy.md
2. factory_method.md
3. adapter.md
4. object_pool.md
5. state.md
6. observer.md
7. producer_consumer.md
8. double_buffer.md
9. command.md
10. singleton.md
11. abstract_factory.md
12. composite.md
13. decorator.md
14. proxy.md
15. facade.md
16. chain_of_responsibility.md
17. template_method.md
18. visitor.md (含 variant 替代)
19. anti_patterns.md

剩余（可选/后续）：
- builder.md
- prototype.md (低优先级, 嵌入式较少)
- flyweight.md (内存共享, 若需)
- mediator.md
- memento.md
- specification.md (可选)
- pimpl.md (结构优化/封装, 可入扩展章节)

优先级建议（下一迭代）：
1. decorator（已完成）与 proxy/facade 综合案例
2. chain_of_responsibility + command + object_pool 组合示例（消息管线）
3. anti_patterns.md 持续扩展：添加真实度量示例

维护约定：
- 新增模式遵循 _template.md
- 每个模式添加 1~3 道练习题目 stub 到 docs/interactive_stubs.md
- 代码示例保持 C++17 可编译（无异常依赖）

后续行动：
- 整理综合案例：功耗管理/通信帧管线
- 生成速查表：模式 -> 嵌入式关注点 (堆/实时/尺寸)
- 根据读者反馈裁剪低频模式
