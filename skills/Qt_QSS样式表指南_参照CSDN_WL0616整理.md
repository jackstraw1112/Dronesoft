# Qt 样式表（QSS）指南 — 参照 CSDN 博文整理

> **参考来源**：[【QT】史上最全最详细的QSS样式表用法及用例说明](https://blog.csdn.net/WL0616/article/details/129118087)（博主：WL0616）  
> **原文版权声明**：原创文章，遵循 [CC 4.0 BY-SA](http://creativecommons.org/licenses/by-sa/4.0/)，转载需保留原文链接与声明。  
> **本文说明**：下文为根据该文目录与要点**重新归纳、校对与排版**的学习笔记，便于团队查阅；涉及 API 细节请以 [Qt 官方 Style Sheets Reference](https://doc.qt.io/qt-6/stylesheet-reference.html) 为准（Qt5 见 [Qt5 对应页](https://doc.qt.io/qt-5/stylesheet-reference.html)）。

与本仓库其他规范的关系：

- 项目内 objectName / QSS 落地约定见：`skills/Qt控件样式表规则.md`、`skills/Qt控件缩写规则.md`。

---

## 1. QSS 基本语法

Qt 样式表用于自定义部件外观，语法形式与 CSS 类似：

```css
selector {
    attribute: value;
}
```

- **selector**：选择器，如 `QWidget`、`QPushButton`、`QGroupBox` 等。  
- **attribute**：属性，如 `color`、`background-color`、`border`、`padding` 等。  
- **value**：与属性对应的取值。

---

## 2. 选择器

### 2.1 选择器类型（原文归纳）

| 类型 | 示例 | 说明 |
|------|------|------|
| 通用选择器 | `*` | 匹配所有部件 |
| 类型选择器 | `QPushButton` | 匹配该类及其子类实例 |
| 属性选择器 | `QPushButton[flat="false"]` | 匹配 `flat` 属性为 `false` 的按钮 |
| 类选择器 | `.QPushButton` | 仅匹配 `QPushButton` 本类，不含子类 |
| ID 选择器 | `QPushButton#myButton` | 匹配 `objectName` 为 `myButton` 的该类型实例 |
| 后代选择器 | `QDialog QPushButton` | 匹配作为 `QDialog`（任意层级）子孙的按钮 |
| 子选择器 | `QDialog > QPushButton` | 仅匹配 `QDialog` **直接子级**的按钮 |

项目建议：优先使用 **`类型#objectName`**（如 `QLabel#lblTaskId`），避免误伤同类型控件。

### 2.2 复合控件：**子控件**（`::`）

复杂控件常需修饰内部子部分，语法为 **`::sub-control`**（双冒号），例如：

```css
QComboBox::drop-down {
    image: url(dropdown.png);
}
```

常见子控件与含义（摘自原文列举，整理为表）：

| 子控件 | 典型用途 |
|--------|----------|
| `::add-line` / `::sub-line` | `QScrollBar` 跳转一行按钮 |
| `::add-page` / `::sub-page` | `QScrollBar` 滑块与按钮之间的页面区域 |
| `::branch` | `QTreeView` 分支指示器 |
| `::chunk` | `QProgressBar` 进度块 |
| `::close-button` | `QDockWidget` / 选项卡关闭按钮等 |
| `::corner` | `QAbstractScrollArea` 两滚动条交汇角 |
| `::down-arrow` / `::up-arrow` | 下拉箭头、微调箭头等 |
| `::down-button` / `::up-button` | `QSpinBox` / `QScrollBar` 等上下按钮 |
| `::drop-down` | `QComboBox` 下拉框按钮 |
| `::float-button` | `QDockWidget` 浮动按钮 |
| `::groove` | `QSlider` 凹槽 |
| `::handle` | `QScrollBar`、`QSplitter`、`QSlider` 拖动手柄 |
| `::indicator` | `QCheckBox`、`QRadioButton`、可选菜单项、`QGroupBox` 勾选指示器等 |
| `::item` | 列表视图、菜单、菜单栏、`QStatusBar` 等条目 |
| `::menu-indicator` / `::menu-button` / `::menu-arrow` | 带菜单的按钮相关 |
| `::pane` / `::tab-bar` / `::left-corner` / `::right-corner` | `QTabWidget` 框架与边角 |
| `::section` | `QHeaderView` 分段 |
| `::separator` | 菜单、`QMainWindow` 等与分隔条 |
| `::tab` | `QTabBar`、`QToolBox` 等选项卡 |
| `::tear` / `::tearoff` | 撕裂指示、`QMenu` 可撕菜单 |
| `::title` | `QGroupBox`、`QDockWidget` 标题区域 |
| `::scroller` | 菜单或 `QTabBar` 滚动区域 |
| `::text` | 部分 `QAbstractItemView` 文本 |

### 2.3 **伪状态**（`:`）

伪状态用**单冒号**，写在选择器之后，用于限制“仅在某种状态下”生效，例如：

```css
QPushButton:hover {
    color: white;
}
```

原文归纳的用法规则：

1. **否定**：`QPushButton:!hover { color: blue; }`（未悬停时）。  
2. **多状态与（AND）**：`QCheckBox:hover:checked { color: white; }`。  
3. **多状态或（OR）**：用逗号分隔多条规则，例如分别写 `QCheckBox:hover` 与 `QCheckBox:checked`。  
4. **子控件 + 伪状态**：`QComboBox::drop-down:hover { image: url(...); }`。

常见伪状态（原文列表摘要，具体以官方文档为准）：

| 伪状态 | 含义摘要 |
|--------|----------|
| `:active` | 部件位于活动窗口 |
| `:disabled` / `:enabled` | 禁用 / 启用 |
| `:hover` | 鼠标悬停 |
| `:pressed` | 鼠标按下 |
| `:focus` | 有输入焦点 |
| `:checked` / `:unchecked` | 选中 / 未选中 |
| `:indeterminate` | 三态（如部分勾选） |
| `:selected` | 选中（选项卡、菜单项等） |
| `:default` | 默认按钮/默认动作等 |
| `:flat` | 扁平按钮等 |
| `:open` / `:closed` | 展开/折叠、下拉打开等 |
| `:read-only` / `:editable` | 只读或可编辑（如 `QLineEdit`、`QComboBox`） |
| `:horizontal` / `:vertical` | 水平/垂直方向 |
| `:top` / `:bottom` / `:left` / `:right` | 方位（如 `QTabBar`） |
| `:first` / `:last` / `:middle` / `:only-one` | 列表中位置 |
| `:alternate` | 交替行着色 |
| 以及 `:window`、`:maximized`、`:minimized`、`:has-children`、`:has-sibling` 等 | 见官方 Style Sheet 文档 |

**注意（原文强调）**：`QLabel` **不支持** `:hover` 伪状态。

---

## 3. 可设置样式的部件（原文表 — 精要摘录）

以下为博文中“各 Widget 如何设置”的**压缩版**，详细子控件与伪状态仍以官方文档为准。

| 部件 | 要点 |
|------|------|
| `QWidget` | 仅部分背景相关属性；**自定义 `QWidget` 子类**若要用 QSS 背景等，通常需在 `paintEvent` 中配合样式绘制，且建议定义 **`Q_OBJECT`**。 |
| `QAbstractScrollArea` | 支持 **盒模型**（margin / border / padding / content）；默认 margin、border-width、padding 多为 0。 |
| `QCheckBox` / `QRadioButton` | 盒模型；指示器用 `::indicator`；`spacing` 控制指示器与文字间距。 |
| `QComboBox` | 外框盒模型；`::drop-down`、`::down-arrow`；可编辑时有 `:editable` 等。 |
| `QDateEdit` / `QDateTimeEdit` / `QTimeEdit` | 参照 `QSpinBox`。 |
| `QDialog` | 继承 `QWidget`，同 `QWidget` 限制。 |
| `QDialogButtonBox` | `button-layout` 调整按钮排布风格。 |
| `QDockWidget` | `border`；`::title`；关闭/浮动子控件；`:vertical`、`:closable`、`:floatable`、`:movable` 等。 |
| `QDoubleSpinBox` | 同 `QSpinBox`。 |
| `QFrame` / `QLabel` | 盒模型；设样式表可能影响 `frameStyle`（文中提到自 4.3 起与 `StyledPanel` 相关行为）。 |
| `QGroupBox` | 盒模型；`::title`；可选时用 `::indicator`；`spacing`。 |
| `QHeaderView` | 盒模型；`::section` 多种伪状态；排序 `::up-arrow` / `::down-arrow`。 |
| `QLineEdit` | 盒模型；`selection-color`、`selection-background-color`；密码相关专用属性。 |
| `QListView` / `QListWidget` | 交替行 `alternate-background-color`；选中样式；`::item`。 |
| `QMainWindow` | `::separator`（与停靠相关）。 |
| `QMenu` / `QMenuBar` | `::item`、`::indicator`、`::separator`、`::right-arrow` 等；多种伪状态。 |
| `QMessageBox` | `message-box-text-interaction-flags` 等。 |
| `QProgressBar` | `::chunk`；不确定进度 `:indeterminate`；`text-align`。 |
| `QPushButton` | `:default`、`:flat`、`:checked`；带菜单 `::menu-indicator`；`:open`/`:closed`。**仅设背景色可能不显示，需配合 `border` 等（原文警告）**。 |
| `QScrollBar` | `:horizontal`/`:vertical`；`::handle`、`::add-line`、`::sub-line`、箭头子控件、`::add-page`、`::sub-page`。 |
| `QSlider` | `::groove`、`::handle`；水平需 `min-width`/`height`，垂直需 `min-height`/`width`（原文要点）。 |
| `QSpinBox` | `::up-button`、`::up-arrow`、`::down-button`、`::down-arrow`。 |
| `QSplitter` | `::handle`。 |
| `QStatusBar` | 背景；`::item`。 |
| `QTabBar` / `QTabWidget` | `::tab`、`::pane`、`::tear`；多种位置与选中伪状态；`QTabBar` 滚动区等。 |
| `QTableView` | 交替行、选中、`gridline-color`；拐角按钮样式注意与 `border`（原文警告）。 |
| `QTextEdit` | 选中前景/背景样式。 |
| `QToolBar` | 方位伪状态；`::separator`、`::handle`。 |
| `QToolButton` | 类似按钮的菜单指示器等；**仅背景需 `border` 等（原文警告）**。 |
| `QToolBox` | `::tab` 与子状态。 |
| `QToolTip` | `opacity`。 |
| `QTreeView` / `QTreeWidget` | 交替行、选中、`::branch`、`:open`/`:closed` 等，`::item`。 |

---

## 4. 属性列表（分类速查）

原文将大量属性对照官方参考与 CSS 概念做了说明；此处按**功能分类**列出常见项，便于检索。标注 `*` 的表示**仅部分控件**支持，以官方表为准。

### 4.1 背景与图像

- `background`、`background-color`、`background-image`  
- `background-repeat`、`background-position`、`background-attachment`  
- `background-clip`（裁剪到 border / padding / content）  
- `background-origin`（相对 border / padding / content 盒定位）

### 4.2 边框与圆角

- `border`、`border-top` / `right` / `bottom` / `left`  
- `border-color`、`border-style`、`border-width`（及分边）  
- `border-image`、`border-radius` 与四角分半径  

### 4.3 尺寸与盒模型

- `margin`、`padding`（及分边）  
- `width`、`height`、`min-width`、`max-width`、`min-height`、`max-height`  
- `spacing`（部分控件内部间距）

### 4.4 布局与子控件几何（常用于 `::xxx`）

- `top`、`right`、`bottom`、`left` — 相对父/参考矩形偏移  
- `subcontrol-origin`、`subcontrol-position` — 子控件原点与对齐  
- `position`：`relative` / `absolute`（原文表中拼写曾为 `posotion`，以文档为准）

### 4.5 字体与文本

- `font`、`font-family`、`font-size`、`font-style`、`font-weight`  
- `color`、`text-align`、`text-decoration`  

### 4.6 选择与列表/表格

- `selection-background-color`、`selection-color`*  
- `alternate-background-color`、`gridline-color`*（表格网格）  
- `show-decoration-selected`*  

### 4.7 其他（原文提及）

- `icon`、*`icon-size`、`image`、`image-position`  
- `opacity`*（如 `QToolTip`）  
- `outline` 系列  
- `-qt-background-role`、`-qt-style-features`（Qt 扩展）  
- 各控件专有：如 `lineedit-password-character`、*`button-layout`、`message-box-text-interaction-flags`* 等  

更完整的“属性名 | 类型 | 说明”表请直接查阅：  
[Qt Style Sheets Reference](https://doc.qt.io/qt-6/stylesheet-reference.html)。  
若需对照 CSS 语义，可参考 [W3School CSS 参考](https://www.w3school.com.cn/cssref/index.asp)（Qt 仅实现子集且与控件绑定）。

---

## 5. 冲突解决（特异性）

当多条规则对**同一属性**给出不同值时，按**选择器更具体者优先**：

```css
QPushButton {
    color: red;
}
QPushButton#okButton {
    color: gray;
}
```

`#okButton` 更具体，故该按钮文字为灰色。  
**一般规律**：ID/属性选择器 > 仅类型选择器；带**伪状态**的规则通常比不带伪状态更具体（原文结论；复杂情况以官方级联规则为准）。

---

## 6. 示例与官方用例

- Qt5：[Stylesheet Examples](https://doc.qt.io/qt-5/stylesheet-examples.html)  
- Qt6：[Stylesheet Examples](https://doc.qt.io/qt-6/stylesheet-examples.html)  

---

## 7. 全局加载 QSS 与 `setStyleSheet` 注意事项（原文第 6 节归纳）

### 7.1 从文件加载（示例骨架）

```cpp
void MainWindow::loadStyleSheet(const QString &styleSheetFile)
{
    QFile file(styleSheetFile);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::information(this, tr("提示"), tr("无法打开样式表文件。"));
        return;
    }
    const QString styleSheet = QString::fromUtf8(file.readAll());
    file.close();
    setStyleSheet(styleSheet);
}
```

也可对 `QApplication::setStyleSheet(...)` 设置**应用级**全局样式。

### 7.2 覆盖规则

- **同一对象**多次调用 `setStyleSheet`：**后一次完全覆盖前一次**，不是追加。  
- **不同对象**分别 `setStyleSheet`：互不影响。  

若需“追加”，应自行读取当前 `styleSheet()` 与文件内容拼接后再 `setStyleSheet`（需注意维护成本）。

---

## 8. 修订说明

本文在整理时对原文抓取文本中的明显笔误做了校正（例如 `QAbstractItemVIew`、`:: sub-line`、部分属性名的空格断开等），并与 Qt 文档命名对齐。若与你的 Qt 版本行为不一致，以对应版本的官方文档为准。
