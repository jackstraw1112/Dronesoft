# Qt代码命名规则（Dronesoft）

> 目标：统一 `source/` 与 `source/01-TaskPlan/` 下 Qt/C++ 代码命名，降低协作成本与重构风险。  
> 适用范围：`*.h`、`*.cpp`、`*.ui` 及 Qt 信号槽相关命名。  
> 对齐风格：与 `.clang-format` 的“可读、稳定、最小 diff”原则一致。

## 1. 通用原则

1. 名称必须表达业务语义，禁止无意义缩写（如 `tmp1`、`data2`）。
2. 同一语义在全项目保持同一命名（如 `taskId` 不混用 `missionId`）。
3. 优先英文命名，UI 文本可以中文，代码标识符统一英文。
4. 布尔变量必须使用可判定前缀：`is/has/can/should`。
5. 一律使用 ASCII 字符作为标识符。

## 2. 大小写与命名风格

- **类名/结构体名/枚举名**：`UpperCamelCase`  
  例：`MissionPlanner`、`TaskItemWidget`、`AreaTargetInfo`
- **函数名/成员函数名/普通变量名**：`lowerCamelCase`  
  例：`setSelectedTask()`、`updateTaskItemWidthConstraints()`
- **常量（`constexpr` / `const` 全局）**：`kUpperCamelCase`  
  例：`kDefaultTimeoutMs`
- **宏**：`UPPER_SNAKE_CASE`（仅在必要时使用）  
  例：`TASK_LIST_MAX_COUNT`

## 3. 文件命名规则（与当前项目保持一致）

1. 类名与文件名保持同名：`ClassName.h/.cpp/.ui`。
2. Qt Designer 生成类对应 `ui_ClassName.h`（由构建系统自动生成，不手改）。
3. 目录内同类功能文件保持前缀一致，例如：
   - `RZTaskListWidget.h/.cpp/.ui`
   - `SetAreaTargetEditDialog.h/.cpp/.ui`

## 4. 类与结构体命名

1. `QWidget/QDialog` 派生类：后缀建议使用 `Widget` / `Dialog`。  
   例：`TaskItemWidget`、`PointTargetEditDialog`
2. 数据承载结构体：后缀统一 `Info` / `Data`。  
   例：`TaskBasicInfo`、`TaskPlanningData`
3. 枚举类型：建议后缀 `Type`（语义分类）或业务名。  
   例：`ThreatLevelType`、`AreaGeometryType`

## 5. 函数命名

1. 动词开头，体现动作语义：`set/get/update/init/reset/add/remove`。
2. 初始化函数固定前缀：
   - `initParams()`
   - `initObject()`
   - `initConnect()`
3. 事件处理函数：
   - Qt 自动连接槽：`on_<objectName>_<signal>()`
   - 手动绑定内部处理：`onXxxClicked()`、`onXxxChanged()`
4. 查询函数使用 `is/has/can`，避免返回 `bool` 但名称像动作函数。  
   例：`hasSelectedTask()`

## 6. 变量命名

### 6.1 成员变量

1. 统一使用 `m_` 前缀：`m_selectedTaskId`、`m_taskItems`。
2. 指针类型也保留语义名，不使用 `p`、`ptr` 作为前缀噪音。

### 6.2 局部变量

1. 使用完整业务语义：`deletedTaskId` 优于 `id`。
2. 作用域很短时可用简名（如循环变量 `i`、迭代器 `it`）。

### 6.3 布尔变量

1. 必须可读成判断句：`isValid`、`hasSelection`、`canDelete`。

## 7. Qt对象名（`*.ui` 的 objectName）规则

> 推荐格式：`<业务语义><控件缩写>`，使用 `lowerCamelCase`。  
> 缩写参考 `skills/Qt控件缩写.md`。
>
> **命名方向强制要求（固化）**：
> - 统一使用：**语义在前，类型在后**。
> - 禁止使用：类型前缀风格（如 `btnSaveTask`、`lblTitle`、`wdMain`）。
> - 历史代码如需调整，按“改动即清理”原则逐步迁移。

示例：
- `saveTaskBtn`（`QPushButton`）
- `taskNameEdit`（`QLineEdit`）
- `taskTypeCombo`（`QComboBox`）
- `taskTableTbw`（`QTableWidget`，若需强调类型）
- `mainLy`（`QLayout`）
- `titleWd`（`QWidget`）
- `taskListBottomSpc`（`QSpacerItem`）
- `setAreaTargetEditDialogDlg`（`QDialog` 根对象）

约束：
1. 同一页面同类控件按语义区分，不使用 `pushButton_2`、`lineEdit_3`。
2. 编辑/删除按钮对称命名：
   - `addAreaTargetBtn`
   - `editAreaTargetBtn`
   - `deleteAreaTargetBtn`
3. root objectName 也遵循相同方向：
   - `QWidget`：`taskListPageWd`
   - `QDialog`：`setPointTargetEditDialogDlg`
4. 布局与间隔器命名：
   - `QLayout`：`xxxLy`（如 `actionsLy`、`gridFormLy`）
   - `QSpacerItem`：`xxxSpc`（如 `actionsLeftSpc`）

## 8. 信号与槽命名

### 8.1 signal

1. 使用“事件已发生”语义，建议过去式：`taskSelected`、`deleteTaskClicked`。
2. 尽量携带必要上下文参数，避免外部再查全局状态。  
   例：`taskSelected(const QString &taskId)`

### 8.2 slot / 内部处理函数

1. 对外部事件响应建议 `onXxx...` 前缀。  
   例：`onDeleteTaskClicked()`
2. 私有辅助函数避免 `on` 前缀，直接描述行为。  
   例：`updateBadge()`、`setPointTargetRow(...)`

## 9. 容器与复数语义

1. 列表/映射变量使用复数：`m_pointTargets`、`m_areaTargets`。
2. 单个对象使用单数：`targetInfo`、`selectedTaskId`。
3. `QMap/QHash` 命名应体现键值语义。  
   例：`m_taskItems`（key=`taskId`, value=`TaskItemWidget*`）。

## 10. 反例与修正

- `btn1` -> `saveTaskBtn`
- `btnSaveTask` -> `saveTaskBtn`
- `lblTitle` -> `titleLbl`
- `wdTitle` -> `titleWd`
- `lyMain` -> `mainLy`
- `spcActionsLeft` -> `actionsLeftSpc`
- `tableWidget_2`（代码中直接使用） -> 建议改为 `pointTargetTable`（渐进重构）
- `DoDelete()` -> `onDeleteTaskClicked()`
- `flag` -> `hasSelection`

## 11. 落地建议（渐进式）

1. 新代码严格遵循本规范。
2. 老代码在“改动即清理”原则下逐步替换，不做一次性大重命名。
3. 每次重命名优先保证：
   - 信号槽连接不破坏；
   - `.ui` 的 objectName 与代码引用一致；
   - 提交粒度小，便于评审与回滚。

## 12. 注释案例（结合命名规则）

> 说明：命名规则负责“是什么”，注释补充“为什么”。  
> 以下案例可直接复制到 Qt/C++ 代码中使用。

### 12.1 函数级注释案例

```cpp
// 设置选中任务：切换卡片高亮并发射 taskSelected 信号。
// 边界：当 taskId 不存在时直接返回，避免非法状态写入。
void setSelectedTask(const QString &taskId);
```

```cpp
// 更新区域目标操作按钮状态。
// 规则：仅当 areaTargetTable 存在有效选中行时，允许编辑和删除。
void updateAreaTargetActionBtnState();
```

### 12.2 信号槽注释案例

```cpp
// 点击删除按钮后统一走 onDeleteTaskClicked，避免散落删除逻辑。
connect(ui->deleteTaskBtn, &QPushButton::clicked,
        this, &RZTaskListWidget::onDeleteTaskClicked);
```

```cpp
// 行双击直接进入编辑流程，保持与“编辑区域目标”按钮行为一致。
connect(ui->areaTargetTable, &QTableWidget::cellDoubleClicked,
        this, [this](int row, int) { editAreaTargetAtRow(row); });
```

### 12.3 数据映射注释案例

```cpp
// row 与 m_areaTargets 索引一一对应，插入/删除必须同步修改两侧数据。
ui->areaTargetTable->insertRow(row);
m_areaTargets.append(targetInfo);
```

```cpp
// deletedTaskId 在删除前缓存，确保 deleteTaskClicked 能拿到稳定任务标识。
const QString deletedTaskId = m_selectedTaskId;
```

### 12.4 命名 + 注释协同案例

```cpp
// hasSelection 表示“是否存在有效选中项”，命名可直接用于条件判断。
const bool hasSelection = (ui->pointTargetTable->currentRow() >= 0);
ui->deletePointTargetBtn->setEnabled(hasSelection);
```

```cpp
// m_taskItems 使用复数命名，表明其为 taskId->TaskItemWidget 的集合容器。
QMap<QString, TaskItemWidget *> m_taskItems;
```

### 12.5 反例与修正（注释层面）

- 反例：`// 点击按钮`（无信息增量）  
  修正：`// 点击保存按钮后触发任务校验与落盘流程。`
- 反例：`// flag 为 true 时删除`  
  修正：`// hasSelection 为 true 时允许删除，避免空删除触发。`
- 反例：`// 处理数据`  
  修正：`// 将表格行数据反序列化为 AreaTargetInfo，并做 targetId 去重校验。`

