# Patterns 目录说明 (已更新)

已有：
1. strategy.md
2. factory_method.md
3. adapter.md
4. object_pool.md
5. state.md
6. observer.md
7. producer_consumer.md
8. double_buffer.md
9. command.md

计划（下一步）：
10. singleton.md (嵌入式安全变体)
11. abstract_factory.md
12. adapter.md (已完成)
13. composite.md
14. object_pool.md (已完成)
15. state.md (已完成)
16. observer.md (已完成)
17. command.md (已完成)
18. producer_consumer.md (已完成)
19. double_buffer.md (已完成)
20. decorator.md
21. proxy.md
22. facade.md
23. chain_of_responsibility.md
24. template_method.md
25. visitor.md (简化+variant 替代)
26. anti_patterns.md (汇总误用与替代)

优先级建议（后续批次）：
- 功耗/硬件相关：singleton（硬件资源管理正确姿势）、proxy（访问权限或延迟硬件访问）、facade（子系统初始化）
- 复杂管线：chain_of_responsibility / decorator
- 数据结构演进：composite (菜单/树形配置)
- 语言特性对比：visitor vs variant

维护约定：
- 新模式对照 _template.md
- 每个模式完成后添加对应练习题 stub（docs/interactive_stubs.md 扩展）
