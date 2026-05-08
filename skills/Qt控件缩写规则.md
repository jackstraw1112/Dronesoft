| 控件           | 缩写  | 控件             | 缩写  |
| ------------ | --- | -------------- | --- |
| QLabel       | lbl | QRadioButton   | rdo |
| QCheckBox    | chk | QGroupBox      | gb  |
| QPushButton  | btn | QToolBox       | tb  |
| QSpinBox     | spb | QStackedWidget | stk |
| QToolButton  | tbn | QFrame         | frm |
| QComboBox    | cmb | QSlider        | sld |
| QLineEdit    | le  | QScrollBar     | scb |
| QDateTimeEdit | dte | QTextEdit     | txe |
| QScrollArea   | sra | QDoubleSpinBox | spb |
| QTabWidget   | tw  | QTextBrowser   | txb |
| QWidget      | wd  | QLayout        | ly  |
| QDialog      | dlg | QProcessBar    | prb |
| QTableWidget | tbw | QListWidget    | lsw |
| QTableView   | tbv | QListView      | lsv |
| QTreeView    | tv  | QTreeWidget    | tw  |

## objectName 组合规则（固化）

- 统一格式：**`<控件缩写><业务语义>`**，整体使用 `lowerCamelCase`。
- 命名方向：**控件缩写在前，业务语义在后**（匈牙利式控件前缀），便于在列表/代码补全中按控件类型聚合浏览。
- 禁止方向：语义在前、缩写在后（如 ~~`saveTaskBtn`~~、~~`taskNameLe`~~）。

示例：

- `btnSaveTask`（`QPushButton`）
- `lblTaskTitle`（`QLabel`）
- `leTaskName`（`QLineEdit`）
- `cmbTaskType`（`QComboBox`）
- `tbwPointTarget`（`QTableWidget`）
- `lyMainForm`（`QLayout`）
- `wdTaskListPage`（`QWidget`）
- `dlgSetAreaTargetEditDialog`（`QDialog` 根对象，可与类名对齐）

**自动连接槽**：若使用 `on_<objectName>_<signal>()`，对象名为 `btnSaveTask` 时槽名为 `on_btnSaveTask_clicked`；更推荐显式 `connect`，避免过长槽名。
