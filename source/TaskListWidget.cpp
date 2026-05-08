// 任务列表组件实现文件
// 实现任务项组件和任务列表组件的功能

#include "TaskListWidget.h"
#include <QPushButton>
#include <QMouseEvent>
#include <QDebug>

// 任务项组件构造函数：初始化任务卡片界面
// 设置鼠标指针为手型，创建垂直布局包含顶部行（ID和状态）和元信息行
TaskItemWidget::TaskItemWidget(QWidget *parent)
    : QFrame(parent)
    , m_taskId("")
{
    setupUi();
}

// 初始化任务项UI界面：创建布局和子控件
void TaskItemWidget::setupUi()
{
    // 设置对象名称，用于样式表选择器
    setObjectName("taskItem");
    // 设置鼠标指针为手型，提示用户可点击
    setCursor(Qt::PointingHandCursor);

    // 创建主垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 10, 5, 10);  // 左右12像素，上下12像素边距
    mainLayout->setSpacing(10);  // 组件间距10像素

    // 创建顶部行布局（包含任务ID和状态）
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    // 创建任务ID标签：青色等宽字体
    m_taskIdLabel = new QLabel(this);
    m_taskIdLabel->setObjectName("taskIdLabel");
    m_taskIdLabel->setStyleSheet(
        "QLabel#taskIdLabel {"
        "   font-family: 'Consolas', 'Courier New', monospace;"
        "   font-size: 10px;"
        "   color: #00e5ff;"
        "   letter-spacing: 1.5px;"
        "}"
    );

    // 创建状态标签：居中显示，带边框
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFixedSize(60, 28);  // 固定大小80x28像素

    topRow->addWidget(m_taskIdLabel);   // 添加任务ID标签
    topRow->addWidget(m_statusLabel);   // 添加状态标签

    // 创建任务名称标签：加粗显示
    m_taskNameLabel = new QLabel(this);
    m_taskNameLabel->setObjectName("taskNameLabel");
    m_taskNameLabel->setStyleSheet(
        "QLabel#taskNameLabel {"
        "   color: #e0e8f0;"
        "   font-size: 10px;"
        "   font-weight: bold;"
        "}"
    );

    // 创建元信息行布局（包含目标数、架次、时间）
    QHBoxLayout *metaRow = new QHBoxLayout();
    metaRow->setSpacing(20);

    // 创建目标数量标签
    m_targetCountLabel = new QLabel(this);
    m_targetCountLabel->setObjectName("metaLabel");
    m_targetCountLabel->setStyleSheet(
        "QLabel#metaLabel {"
        "   color: #7a8ba8;"
        "   font-size: 10px;"
        "}"
    );

    // 创建飞机架次标签
    m_aircraftCountLabel = new QLabel(this);
    m_aircraftCountLabel->setObjectName("metaLabel");
    m_aircraftCountLabel->setStyleSheet(
        "QLabel#metaLabel {"
        "   color: #7a8ba8;"
        "   font-size: 10px;"
        "}"
    );

    // 创建时间标签
    m_timeLabel = new QLabel(this);
    m_timeLabel->setObjectName("metaLabel");
    m_timeLabel->setStyleSheet(
        "QLabel#metaLabel {"
        "   color: #7a8ba8;"
        "   font-size: 10px;"
        "}"
    );

    metaRow->addWidget(m_targetCountLabel);    // 添加目标数量标签
    metaRow->addWidget(m_aircraftCountLabel);  // 添加飞机架次标签
    metaRow->addWidget(m_timeLabel);           // 添加时间标签
    metaRow->addStretch();                      // 添加弹性空间

    // 将各行布局添加到主布局
    mainLayout->addLayout(topRow);      // 添加顶部行布局
    mainLayout->addWidget(m_taskNameLabel);  // 添加任务名称标签
    mainLayout->addLayout(metaRow);     // 添加元信息行布局

    // 设置任务卡片的基础样式：深色背景、细边框、圆角
    setStyleSheet(
        "QFrame#taskItem {"
        "   background-color: #0d1326;"
        "   border: 1px solid #1a3a6a;"
        "   border-radius: 4px;"
        "   margin-bottom: 8px;"
        "}"
        "QFrame#taskItem:hover {"
        "   border-color: #00b4ff;"
        "}"
    );
}

// 设置任务编号
void TaskItemWidget::setTaskId(QString taskId)
{
    m_taskId = taskId;
    m_taskIdLabel->setText(taskId);
}

// 设置任务名称
void TaskItemWidget::setTaskName(QString taskName)
{
    m_taskNameLabel->setText(taskName);
}

// 设置任务状态：根据状态类型显示不同颜色标签
// 规划中：黄色背景
// 待执行：灰色背景
// 执行中：青色背景
void TaskItemWidget::setTaskStatus(QString status)
{
    m_status = status;

    if (status == "规划中" || status == "planning") {
        // 规划中状态：黄色标签
        m_statusLabel->setText(status == "planning" ? "规划中" : status);
        m_statusLabel->setStyleSheet(
            "QLabel#statusLabel {"
            "   font-size: 10px;"
            "   color: #ffaa44;"
            "   background-color: rgba(255, 170, 68, 0.2);"
            "   border: 1px solid #ffaa44;"
            "   padding: 2px 8px;"
            "   border-radius: 3px;"
            "}"
        );
    } else if (status == "待执行" || status == "ready") {
        // 待执行状态：灰色标签
        m_statusLabel->setText(status == "ready" ? "待执行" : status);
        m_statusLabel->setStyleSheet(
            "QLabel#statusLabel {"
            "   font-size: 10px;"
            "   color: #7a8ba8;"
            "   background-color: rgba(122, 139, 168, 0.2);"
            "   border: 1px solid #7a8ba8;"
            "   padding: 2px 8px;"
            "   border-radius: 3px;"
            "}"
        );
    } else if (status == "执行中" || status == "active") {
        // 执行中状态：青色标签
        m_statusLabel->setText(status == "active" ? "执行中" : status);
        m_statusLabel->setStyleSheet(
            "QLabel#statusLabel {"
            "   font-size: 10px;"
            "   color: #00e5ff;"
            "   background-color: rgba(0, 229, 255, 0.2);"
            "   border: 1px solid #00e5ff;"
            "   padding: 2px 8px;"
            "   border-radius: 3px;"
            "}"
        );
    } else if (status == "已完成" || status == "completed") {
        // 已完成状态：绿色标签
        m_statusLabel->setText(status == "completed" ? "已完成" : status);
        m_statusLabel->setStyleSheet(
            "QLabel#statusLabel {"
            "   font-size: 10px;"
            "   color: #00e676;"
            "   background-color: rgba(0, 230, 118, 0.2);"
            "   border: 1px solid #00e676;"
            "   padding: 2px 8px;"
            "   border-radius: 3px;"
            "}"
        );
    } else {
        // 其他状态：灰色文本
        m_statusLabel->setText(status);
        m_statusLabel->setStyleSheet(
            "QLabel#statusLabel {"
            "   font-size: 10px;"
            "   color: #5a6e75;"
            "   background-color: rgba(90, 110, 117, 0.2);"
            "   border: 1px solid #5a6e75;"
            "   padding: 2px 8px;"
            "   border-radius: 3px;"
            "}"
        );
    }
}

// 设置目标数量
void TaskItemWidget::setTargetCount(int count)
{
    m_targetCountLabel->setText(QString("◉ %1 目标").arg(count));
}

// 设置飞机架次数量
void TaskItemWidget::setAircraftCount(int count)
{
    m_aircraftCountLabel->setText(QString("✈ %1 架次").arg(count));
}

// 设置任务时间
void TaskItemWidget::setTime(QString time)
{
    m_timeLabel->setText(QString("⏱ %1").arg(time));
}

// 设置选中状态：选中时显示金色边框高亮效果
void TaskItemWidget::setSelected(bool selected)
{
    if (selected) {
        // 选中状态：蓝色边框和左侧高亮条
        setStyleSheet(
            "QFrame#taskItem {"
            "   background-color: #0d1326;"
            "   border: 1px solid #00b4ff;"
            "   border-left: 3px solid #00b4ff;"
            "   border-radius: 4px;"
            "   margin-bottom: 8px;"
            "}"
            "QFrame#taskItem:hover {"
            "   border-color: #00e5ff;"
            "   border-left: 3px solid #00e5ff;"
            "}"
        );
    } else {
        // 非选中状态：默认深色边框
        setStyleSheet(
            "QFrame#taskItem {"
            "   background-color: #0d1326;"
            "   border: 1px solid #1a3a6a;"
            "   border-radius: 4px;"
            "   margin-bottom: 8px;"
            "}"
            "QFrame#taskItem:hover {"
            "   border-color: #00b4ff;"
            "}"
        );
    }
}

// 鼠标点击事件处理：左键点击发射clicked信号
void TaskItemWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_taskId);
    }
    QFrame::mousePressEvent(event);
}

// 任务列表组件构造函数
TaskListWidget::TaskListWidget(QWidget *parent)
    : QFrame(parent)
{
    setupUi();

    // 添加测试任务数据
    addTestTasks();
}

// 初始化UI界面：创建头部、滚动区域和底部按钮
void TaskListWidget::setupUi()
{
    // 创建主垂直布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(3, 3, 3, 3);  // 无边距
    m_mainLayout->setSpacing(0);  // 无间距

    // 创建头部框架：深色背景，包含标题和徽章
    m_headerFrame = new QFrame(this);
    m_headerFrame->setObjectName("headerFrame");
    m_headerFrame->setMinimumSize(QSize(0, 54));
    m_headerFrame->setStyleSheet(
        "QFrame#headerFrame {"
        "   background-color: #0d1326;"
        "   border-bottom: 1px solid #1a3a6a;"
        "}"
    );

    // 创建头部水平布局
    QHBoxLayout *headerLayout = new QHBoxLayout(m_headerFrame);
    headerLayout->setContentsMargins(16, 12, 16, 12);

    // 创建标题标签：金色左边框
    QLabel *titleLabel = new QLabel(m_headerFrame);
    titleLabel->setText("任务列表");
    titleLabel->setStyleSheet(
        "QLabel {"
        "   color: #00b4ff;"
        "   font-weight: bold;"
        "   font-size: 16px;"
        "   letter-spacing: 2px;"
        "   border-left: 3px solid #00b4ff;"
        "   padding-left: 8px;"
        "}"
    );

    // 创建徽章标签：显示任务数量
    m_badgeLabel = new QLabel(m_headerFrame);
    m_badgeLabel->setObjectName("badgeLabel");
    m_badgeLabel->setAlignment(Qt::AlignRight);
    updateBadge();

    headerLayout->addWidget(titleLabel);   // 添加标题标签
    headerLayout->addWidget(m_badgeLabel); // 添加徽章标签

    // 创建滚动区域：用于显示任务列表
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("taskScrollArea");
    m_scrollArea->setStyleSheet(
        "QScrollArea#taskScrollArea {"
        "   background-color: transparent;"
        "   border: none;"
        "}"
        "QScrollArea#taskScrollArea > QWidget {"
        "   background-color: transparent;"
        "}"
        "QScrollArea#taskScrollArea QScrollBar:vertical {"
        "   background-color: #0a0e1a;"
        "   width: 6px;"
        "   margin: 0px;"
        "}"
        "QScrollArea#taskScrollArea QScrollBar::handle:vertical {"
        "   background-color: #1a3a6a;"
        "   border-radius: 3px;"
        "   min-height: 20px;"
        "}"
        "QScrollArea#taskScrollArea QScrollBar::handle:vertical:hover {"
        "   background-color: #00b4ff;"
        "}"
        "QScrollArea#taskScrollArea QScrollBar::add-line:vertical,"
        "QScrollArea#taskScrollArea QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
    );
    m_scrollArea->setWidgetResizable(true);  // 内部部件可调整大小
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // 隐藏水平滚动条

    // 创建滚动内容容器
    m_scrollContent = new QWidget(m_scrollArea);
    m_scrollContent->setObjectName("scrollContent");
    m_scrollContent->setStyleSheet(
        "QWidget#scrollContent {"
        "   background-color: transparent;"
        "}"
    );

    // 创建任务列表垂直布局
    m_taskListLayout = new QVBoxLayout(m_scrollContent);
    m_taskListLayout->setContentsMargins(16, 16, 16, 16);
    m_taskListLayout->setSpacing(0);
    m_taskListLayout->addStretch();  // 添加弹性空间使任务项靠上显示

    m_scrollArea->setWidget(m_scrollContent);

    // 创建底部按钮框架
    QFrame *buttonFrame = new QFrame(this);
    buttonFrame->setObjectName("buttonFrame");
    buttonFrame->setStyleSheet(
        "QFrame#buttonFrame {"
        "   background-color: #0a0e1a;"
        "   border-top: 1px solid #1a3a6a;"
        "}"
    );

    // 创建按钮垂直布局
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonFrame);
    buttonLayout->setContentsMargins(16, 12, 16, 12);
    buttonLayout->setSpacing(8);

    // 创建新建任务按钮：金色背景
    QPushButton *newTaskBtn = new QPushButton(buttonFrame);
    newTaskBtn->setText("+ 新建任务 / NEW MISSION");
    newTaskBtn->setCursor(Qt::PointingHandCursor);
    newTaskBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #00b4ff;"
        "   color: #0a0e1a;"
        "   border: none;"
        "   padding: 12px 16px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   letter-spacing: 1px;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #00e5ff;"
        "}"
    );
    connect(newTaskBtn, &QPushButton::clicked, this, &TaskListWidget::newTaskClicked);

    // 创建导入情报按钮：透明背景带边框
    QPushButton *importBtn = new QPushButton(buttonFrame);
    importBtn->setText("导入情报 / IMPORT INTEL");
    importBtn->setCursor(Qt::PointingHandCursor);
    importBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: #7a8ba8;"
        "   border: 1px solid #1a3a6a;"
        "   padding: 12px 16px;"
        "   font-size: 14px;"
        "   letter-spacing: 1px;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0, 180, 255, 0.1);"
        "   border-color: #00b4ff;"
        "   color: #00e5ff;"
        "}"
    );
    connect(importBtn, &QPushButton::clicked, this, &TaskListWidget::importIntelClicked);

    buttonLayout->addWidget(newTaskBtn);  // 添加新建任务按钮
    buttonLayout->addWidget(importBtn);    // 添加导入情报按钮

    // 将各部件添加到主布局
    m_mainLayout->addWidget(m_headerFrame);    // 添加头部框架
    m_mainLayout->addWidget(m_scrollArea, 1);  // 添加滚动区域（可拉伸）
    m_mainLayout->addWidget(buttonFrame);      // 添加底部按钮框架

    // QFrame 边框样式（QFrame 原生支持 border 绘制）
    setFrameShape(QFrame::NoFrame);
    setAutoFillBackground(true);
    setStyleSheet(
        "TaskListWidget {"
        "   background-color: #0d1326;"
        "   border: 2px solid #1a3a6a;"
        "   border-radius: 6px;"
        "}"
    );
}

// 添加任务项：根据提供的信息创建并显示任务卡片
void TaskListWidget::addTask(QString taskId, QString taskName,
                             QString status, int targets, int aircraft,
                             QString time)
{
    // 检查是否已存在相同ID的任务，避免重复添加
    if (m_taskItems.contains(taskId)) {
        return;
    }

    // 创建任务项组件
    TaskItemWidget *item = new TaskItemWidget(m_scrollContent);
    item->setTaskId(taskId);
    item->setTaskName(taskName);
    item->setTaskStatus(status);
    item->setTargetCount(targets);
    item->setAircraftCount(aircraft);
    item->setTime(time);

    // 连接任务项点击信号到列表的槽函数
    connect(item, &TaskItemWidget::clicked, this, &TaskListWidget::onTaskItemClicked);

    // 在弹性空间之前插入任务项
    m_taskListLayout->insertWidget(m_taskListLayout->count() - 1, item);
    // 添加到任务映射表
    m_taskItems.insert(taskId, item);

    // 更新徽章显示
    updateBadge();

    // 如果没有选中的任务，自动选中新添加的任务
    if (m_selectedTaskId.isEmpty()) {
        setSelectedTask(taskId);
    }
}

// 清空所有任务项：删除所有任务卡片并重置选中状态
void TaskListWidget::clearTasks()
{
    // 遍历映射表，删除所有任务项组件
    QMap<QString, TaskItemWidget*>::iterator it = m_taskItems.begin();
    while (it != m_taskItems.end()) {
        delete it.value();
        ++it;
    }
    m_taskItems.clear();       // 清空映射表
    m_selectedTaskId.clear();  // 清空选中状态
    updateBadge();             // 更新徽章
}

// 设置选中的任务：高亮显示指定任务并发射选中信号
void TaskListWidget::setSelectedTask(QString taskId)
{
    // 检查任务是否存在
    if (!m_taskItems.contains(taskId)) {
        return;
    }

    // 取消上一个选中任务的选中状态
    if (!m_selectedTaskId.isEmpty() && m_taskItems.contains(m_selectedTaskId)) {
        m_taskItems.value(m_selectedTaskId)->setSelected(false);
    }

    // 设置新的选中状态
    m_selectedTaskId = taskId;
    m_taskItems.value(taskId)->setSelected(true);
    emit taskSelected(taskId);  // 发射选中信号
}

// 更新任务数量徽章：根据当前任务数量更新显示
void TaskListWidget::updateBadge()
{
    // 统计任务数量
    int activeCount = 0;
    QMap<QString, TaskItemWidget*>::iterator it = m_taskItems.begin();
    while (it != m_taskItems.end()) {
        ++activeCount;
        ++it;
    }

    // 更新徽章文本（格式：XX ACTIVE）
    m_badgeLabel->setText(QString("%1 ACTIVE").arg(activeCount, 2, 10, QChar('0')));
    m_badgeLabel->setStyleSheet(
        "QLabel#badgeLabel {"
        "   font-size: 14px;"
        "   color: #7a8ba8;"
        "   letter-spacing: 1px;"
        "}"
    );
}

// 内部slot：处理任务项点击事件
void TaskListWidget::onTaskItemClicked(QString taskId)
{
    setSelectedTask(taskId);
}

// 添加测试任务数据：用于演示任务列表效果
void TaskListWidget::addTestTasks()
{
    // 添加5个测试任务，覆盖不同的状态
    addTask("MSN-2026-0501-A", "SEAD预先打击任务 - 雷达站群A区",
            "执行中", 8, 12, "2026-05-03 08:30");

    addTask("MSN-2026-0502-B", "对地攻击任务 - 指挥中心B区",
            "规划中", 5, 8, "2026-05-03 14:00");

    addTask("MSN-2026-0503-C", "压制干扰任务 - 通信中继C区",
            "待执行", 3, 6, "2026-05-04 06:00");

}
