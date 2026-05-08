| 控件           | 缩写  | 控件             | 缩写  |
| ------------ | --- | -------------- | --- |
| QLabel       | lbl | QRadioButton   | rdo |
| QCheckBox    | chk | QGroupBox      | gb  |
| QPushButton  | btn | QToolBox       | tb  |
| QSpinBox     | spb | QStackedWidget | stk |
| QToolButton  | tbn | QFrame         | frm |
| QComboBox    | cmb | QSlider        | sld |
| QLineEdit    | le  | QScrollBar     | scb |
| QTabWidget   | tw  | QTextBrowser   | txb |
| QWidget      | wd  | QLayout        | ly  |
| QDialog      | dlg | QProcessBar    | prb |
| QTableWidget | tbw | QListWidget    | lsw |
| QTableView   | tbv | QListView      | lsv |
| QTreeView    | tv  | QTreeWidget    | tw  |

## objectName 组合规则（固化）

- 统一格式：`<业务语义><控件缩写>`（`lowerCamelCase`）。
- 命名方向：**语义在前，缩写在后**。
- 禁止方向：`<控件缩写><业务语义>`（如 `btnSaveTask`、`lblTitle`）。

示例：

- `saveTaskBtn`（QPushButton）
- `titleLbl`（QLabel）
- `taskNameEdit`（QLineEdit）
- `taskTypeCombo`（QComboBox）
- `pointTargetTbw`（QTableWidget）
- `mainLy`（QLayout）
- `titleWd`（QWidget）
- `setAreaTargetEditDialogDlg`（QDialog）

