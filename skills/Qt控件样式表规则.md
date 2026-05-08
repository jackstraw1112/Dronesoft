# Qt控件样式表规则（QSS）

> 目标：规范 `.qss` 的写法与组织方式，保证“样式可读、状态可控、维护成本低”。  
> 说明：本文件结合当前项目中已有的 QSS（`bin_windows/theme/TaskItemWidget.qss`）给出“控件样式模板 + 每个属性的作用说明 + 全局统计”。

---

## 1. QSS 基础规则（必须遵守）

### 1.1 选择器规则

1. **对象选择器（objectName）优先**：使用 `QLabel#xxx` / `QFrame#yyy` 这种写法，确保样式只作用于指定控件。
2. **统一使用 `#` 精确定位控件**：禁止大量依赖无约束的类型选择器（例如单纯 `QLabel { ... }`），避免误伤。
3. **伪状态使用 `:hover` / `:checked` 等**：只用于“鼠标态/控件态”类的临时状态。
4. **动态状态使用属性选择器**：例如
   - `[selected="true"]`：由代码 `setProperty("selected", true/false)` 驱动
   - `[statusType="planning"]`：由代码给 `QLabel` 设置 `setProperty("statusType", "...")` 驱动

### 1.2 分组与去重

1. 多个相同样式的控件，使用逗号分组：`QLabel#a, QLabel#b { ... }`。
2. 相同“基类样式”写在一次选择器里，状态差异只在状态选择器里改动（例如 `lblStatus` 的基础样式放 `QLabel#lblStatus`，状态颜色放 `QLabel#lblStatus[statusType="..."]`）。

### 1.3 禁止在业务代码里写死大量样式

1. 业务/逻辑层只负责：
   - 设置文本（`setText`）
   - 设置 dynamic property（`setProperty`）
   - 触发刷新样式（如必要时调用 `refreshStyle()`）
2. 样式规则集中放在 `.qss`，避免 `setStyleSheet()` 在多处散落造成难维护。

---

## 2. 当前项目 QSS 内容总览（总计）

通过递归筛选 `*.qss`，当前工程实际存在的样式表文件数量为：

- **样式表文件数：1**
- **文件：** `bin_windows/theme/TaskItemWidget.qss`

在该文件中出现的选择器块（按块 `selector { ... }` 计数）：

- **选择器块数：12**

样式属性赋值数量（按每个 `{}` 内出现的 `key: value;` 计数）：

- **属性赋值总数：34**

> 注：如果你把 QSS 从 `bin_windows/theme/` 移回 `source/01-TaskPlan/` 或新增更多 QSS 文件，以上统计会随之变化。

---

## 3. 每种控件样式与属性说明（以当前文件为准）

下面按“选择器块”列出控件样式与属性解释。由于 QSS 语义是通用的，这些属性说明也可以迁移到你未来新增的控件样式块。

### 3.1 `QFrame#taskItem`（卡片容器基础样式）

选择器块：

- `QFrame#taskItem { ... }`

属性说明：

1. `background-color: <color>;`
   - 卡片背景色。
2. `border: <width> <style> <color>;`
   - 边框统一设置（包括宽度、线型、颜色）。
3. `border-radius: <px>;`
   - 圆角半径，视觉上更柔和。
4. `margin-bottom: <px>;`
   - 卡片之间的底部间距（用于垂直列表排布）。

常见写法建议：
1. `border` 与 `[selected="true"]` 的 `border` 建议保持相同的“宽度 + 线型”，只改颜色/边左宽度。
2. `margin-bottom` 只在列表纵向布局需要间距时使用；如果外层布局已经控制 spacing/margins，则避免重复间距。

---

### 3.2 `QFrame#taskItem:hover`（鼠标悬停态）

选择器块：

- `QFrame#taskItem:hover { ... }`

属性说明：

1. `border-color: <color>;`
   - 仅改变边框颜色，其它保持不变，避免造成布局抖动。

常见写法建议：
1. hover 态尽量只改颜色，不改 `border-width`、`padding`、`margin`，减少“抖动”。

---

### 3.3 `QFrame#taskItem[selected="true"]`（选中态）

选择器块：

- `QFrame#taskItem[selected="true"] { ... }`

属性说明：

1. `border: ...;`
   - 选中边框颜色/线型整体变更。
2. `border-left: <width> <style> <color>;`
   - 使用“左侧加粗边”突出选中态，常用于列表当前行/当前卡片高亮。

常见写法建议：
1. `border-left` 的宽度要与基础 `border` 的宽度保持视觉一致（例如基础 1px，左侧加粗 3px）。
2. 若你在代码里同时维护 `selected` dynamic property，请确保刷新样式逻辑能及时生效（避免用户观察不到状态变化）。

---

### 3.4 `QLabel#lblTaskId`（任务编号标签）

选择器块：

- `QLabel#lblTaskId { ... }`

属性说明：

1. `font-family: <family-list>;`
   - 字体家族。这里使用等宽字体用于编号对齐。
2. `font-size: <px>;`
   - 字号。
3. `color: <color>;`
   - 字体颜色。
4. `letter-spacing: <px>;`
   - 字符间距，提升编号可读性。

常见写法建议：
1. “等宽字体 + letter-spacing”适合编号/坐标类展示。

---

### 3.5 `QLabel#lblTaskName`（任务名称标签）

选择器块：

- `QLabel#lblTaskName { ... }`

属性说明：

1. `color: <color>;`
   - 字体颜色。
2. `font-size: <px>;`
   - 字号。
3. `font-weight: bold | normal | <number>;`
   - 字重，让标题信息层级更明确。

---

### 3.6 `QLabel#lblTargetCount, QLabel#lblThreatLevel, QLabel#lblTime`

选择器块：

- `QLabel#lblTargetCount, QLabel#lblThreatLevel, QLabel#lblTime { ... }`

属性说明：

1. `color: <color>;`
2. `font-size: <px>;`

设计意义：
1. 让同一“元信息区”控件看起来属于同一信息层级，减少视觉噪声。

---

### 3.7 `QLabel#lblStatus`（状态标签基础样式）

选择器块：

- `QLabel#lblStatus { ... }`

属性说明：

1. `font-size: <px>;`
2. `padding: <v> <h>;`
   - 上下左右内边距（常用于做“标签胶囊”效果）。
3. `border-radius: <px>;`
   - 圆角，让状态标签像 pill。

---

### 3.8 `QLabel#lblStatus[statusType="..."]`（状态颜色映射）

选择器块（当前有 5 种）：

- `QLabel#lblStatus[statusType="planning"]`
- `QLabel#lblStatus[statusType="ready"]`
- `QLabel#lblStatus[statusType="active"]`
- `QLabel#lblStatus[statusType="completed"]`
- `QLabel#lblStatus[statusType="other"]`

属性说明（每个状态块一致）：

1. `color: <color>;`
   - 标签文字颜色。
2. `background-color: rgba(r, g, b, a);`
   - 标签背景色，使用透明度 `a` 与全局 UI 更融合。
3. `border: 1px solid <color>;`
   - 给标签加边框，让颜色在暗色背景下更清晰。

状态驱动规则（与代码强绑定）：
1. 你需要在 `TaskItemWidget.cpp`（或类似逻辑）里将 `statusType` dynamic property 设置为对应字符串。
2. 字符串必须与 QSS 选择器里的值一致（如 `"planning"`, `"ready"`）。

---

## 4. 样式表编写规则与模板（建议你未来照着做）

### 4.1 写样式的“最小模板”

1. 容器基础态（`objectName`）
2. 容器伪态（`:hover`）
3. 容器 dynamic 态（`[prop="value"]`）
4. 子控件基础样式（`QLabel#...`）
5. 子控件动态映射（`QLabel#...[statusType="..."]`）

### 4.2 命名规则（与控件缩写规则一致）

1. objectName 采用 `skills/Qt控件缩写规则.md` 的缩写风格：例如 `lblTaskId` / `lblStatus`。
2. QSS 选择器必须与 `.ui` 中的 `objectName` 完全一致。

### 4.3 常见错误清单

1. QSS 选择器写错 `objectName`（导致样式“看起来没生效”）。
2. dynamic property 的 value 与 QSS 不一致（例如代码 set 成 `"plan"`，QSS 选择器写成 `"planning"`）。
3. 在 hover/selected 态里修改 `margin` 或 `padding`，导致 UI 抖动。

---

## 5. 与现有代码的协作点（你需要知道的“接口”）

以当前项目为例，你的代码需要做到两点：

1. 容器/控件设置 `objectName`（例如卡片容器 `setObjectName("taskItem")`）
2. 给 `QFrame` 或 `QLabel` 设置 dynamic properties：
   - `setProperty("selected", true/false)`
   - `ui->lblStatus->setProperty("statusType", "planning"/"ready"/"active"/"completed"/"other")`

只要这些“协作点”一致，QSS 就会自动完成状态视觉映射。

