# Qt代码风格规则（Dronesoft）

> 目标：统一 Qt/C++ 代码排版与书写习惯，保证“高可读、低噪声、最小 diff”。  
> 依据：`/.clang-format`（LLVM 基线 + 项目定制）。  
> 适用范围：`source/`、`source/01-TaskPlan/` 下 `*.h`、`*.cpp`、`*.ui` 关联代码。

## 1. 核心原则

1. **格式自动化优先**：能交给 `clang-format` 的排版，不做手工对齐。
2. **语义优先于凑行**：本项目 `ColumnLimit: 0`，不强制断行，按语义组织代码。
3. **最小改动原则**：避免“仅对齐/仅换行”的大面积无业务变更提交。
4. **一致性优先**：同类结构使用同一写法（括号、缩进、空格、换行）。

## 2. 与 `.clang-format` 对齐的关键格式

### 2.1 缩进与空行

1. 缩进宽度：4 空格（`IndentWidth: 4`，`TabWidth: 4`）。
2. 命名空间内部继续缩进（`NamespaceIndentation: All`）。
3. 函数/类等定义块之间保留一个空行（`SeparateDefinitionBlocks: Always`）。
4. 文件末尾必须有换行（`InsertNewlineAtEOF: true`）。

### 2.2 花括号风格（Allman）

采用自定义 Allman 风格（`BreakBeforeBraces: Custom`）：

1. `class/struct/enum/namespace/function/control` 的 `{` 另起一行。
2. `else/catch/while`（do-while）前保持换行。
3. 短函数、短 if、短 lambda **不压缩成单行**。

示例：

```cpp
if (isValid)
{
    saveTask();
}
else
{
    showWarning();
}
```

### 2.3 括号与空格

1. 函数调用、条件判断括号内不加空格：`func(a, b)`、`if(a)`。
2. 空参数括号不加空格：`reset()`。
3. 模板尖括号不加空格：`QList<TaskData>`。
4. C 风格转换括号内不加空格：`(int)value`。

### 2.4 参数换行与续行

1. 参数尽量同一行（`BinPackArguments/Parameters: true`）。
2. 续行缩进 8 空格（`ContinuationIndentWidth: 8`）。
3. 高惩罚项让“首参数前断行”尽量少发生（`PenaltyBreakBeforeFirstCallParameter` 很高）。

### 2.5 include 顺序

按分组排序：

1. 系统头：`<...>`
2. 本地头：`"..."`
3. 其他兜底

同组内按字典序排列；不要手工插入“视觉对齐空格”。

## 3. Qt/C++ 书写规则（人工约束）

以下规则是对自动格式化的补充，`clang-format` 无法完全保证。

### 3.1 函数实现风格

1. 单一职责：一个函数只处理一个清晰动作。
2. 复杂函数采用固定阶段：`initParams -> initObject -> initConnect`。
3. 先做边界返回，再走主流程（guard clause）。

示例：

```cpp
void TaskListWidget::onDeleteTaskClicked()
{
    if (m_selectedTaskId.isEmpty() || !m_taskItems.contains(m_selectedTaskId))
    {
        return;
    }

    // 主流程...
}
```

### 3.2 信号槽风格

1. `connect` 使用函数指针新语法，避免字符串宏写法。
2. 多个 `connect` 按“UI按钮 -> 列表事件 -> 跨组件信号”分组。
3. lambda 槽仅用于短逻辑；超过 5~10 行建议提取私有函数。

### 3.3 条件与枚举

1. 禁止魔法字符串/魔法数字散落，优先枚举和常量。
2. 枚举转换统一走 `TaskPlanningTypeConvert.h`。
3. 多分支状态判断优先 `if/else if` 按“常见到少见”排序。

### 3.4 容器与循环

1. Qt 容器遍历优先清晰可读，允许迭代器方式。
2. 删除元素时显式处理选中态、索引映射和后续刷新。
3. 批量更新 UI 时，先更新数据，再统一刷新界面状态。

### 3.5 UI 相关风格

1. UI 结构在 `.ui` 中定义，`.cpp` 只写逻辑与状态同步。
2. 样式优先放 `.qss`，避免在逻辑代码里大量 `setStyleSheet`。
3. 尺寸策略、对齐策略（如居中）必须在代码中有明确意图注释。

## 4. 提交前检查清单

1. 是否执行过格式化（遵循项目 `.clang-format`）？
2. 是否存在仅排版但无业务价值的大面积改动？
3. `include` 顺序是否符合分组规则？
4. 是否引入了新的魔法值/魔法字符串？
5. 注释是否解释了“为什么”，而不是翻译代码？

## 5. 与其他规范关系

本规范关注“怎么写得一致”；命名与注释请配合以下文档：

- `skills/Qt代码命名规则.md`
- `skills/Qt代码注释规则.md`
- `skills/Qt控件缩写规则.md`

当规则冲突时，优先级如下：

1. 项目 `.clang-format`
2. 本文档（代码风格）
3. 命名/注释规则文档

