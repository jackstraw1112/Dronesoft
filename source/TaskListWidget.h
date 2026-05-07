// 任务列表组件头文件
// 任务列表组件用于显示和管理任务项，支持任务的添加、选择和删除操作
// 每个任务项包含任务ID、名称、状态、目标数、架次数和时间等基本信息

#ifndef TASKLISTWIDGET_H
#define TASKLISTWIDGET_H

// Qt核心组件
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QVariant>
#include <QVector>
#include <QMap>

// 任务项组件类：显示单个任务卡片的完整信息
// 包含任务ID、状态标签、任务名称、目标数量、飞机架次和时间信息
class TaskItemWidget : public QFrame
{
    Q_OBJECT

public:
    // 构造函数：创建任务项组件，可指定父窗口
    explicit TaskItemWidget(QWidget *parent = nullptr);

    // 初始化UI界面：创建布局和子控件
    void setupUi();

    // 设置任务编号（显示为青色等宽字体）
    void setTaskId(QString taskId);

    // 设置任务名称（显示为加粗文本）
    void setTaskName(QString taskName);

    // 设置任务状态：根据状态类型显示不同颜色标签
    // 支持状态：规划中（黄色）、待执行（灰色）、执行中（青色）
    void setTaskStatus(QString status);

    // 设置目标数量（显示格式：◉ X 目标）
    void setTargetCount(int count);

    // 设置飞机架次数量（显示格式：✈ X 架次）
    void setAircraftCount(int count);

    // 设置任务时间（显示格式：⏱ XX:XX）
    void setTime(QString time);

    // 获取任务编号
    QString getTaskId() { return m_taskId; }

    // 设置选中状态：选中时显示金色边框高亮效果
    void setSelected(bool selected);

signals:
    // 任务项被点击时发射的信号，包含任务ID
    void clicked(QString taskId);

protected:
    // 鼠标点击事件处理：左键点击发射clicked信号
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString m_taskId;                     // 任务唯一标识符
    QString m_status;                     // 任务状态
    QLabel *m_taskIdLabel;                // 任务ID标签（青色等宽字体）
    QLabel *m_statusLabel;                // 任务状态标签（带颜色背景）
    QLabel *m_taskNameLabel;              // 任务名称标签
    QLabel *m_targetCountLabel;           // 目标数量标签
    QLabel *m_aircraftCountLabel;         // 飞机架次标签
    QLabel *m_timeLabel;                  // 任务时间标签
};

// 任务列表组件类：管理多个任务项的容器组件
// 提供任务列表的滚动显示、新建任务和导入情报功能
class TaskListWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数：创建任务列表组件，初始化UI布局
    explicit TaskListWidget(QWidget *parent = nullptr);

    // 添加任务项：根据提供的信息创建并显示任务卡片
    // 参数：任务ID、名称、状态、目标数量、飞机架次、时间
    void addTask(QString taskId, QString taskName,
                 QString status, int targets, int aircraft,
                 QString time);

    // 清空所有任务项：删除所有任务卡片并重置选中状态
    void clearTasks();

    // 设置选中的任务：高亮显示指定任务并发射选中信号
    void setSelectedTask(QString taskId);

    // 获取当前任务数量
    int getTaskCount() { return m_taskItems.size(); }

    // 添加测试任务数据：用于演示任务列表效果
    void addTestTasks();

signals:
    // 任务被选中时发射的信号，包含任务ID
    void taskSelected(QString taskId);

    // 新建任务按钮被点击时发射的信号
    void newTaskClicked();

    // 导入情报按钮被点击时发射的信号
    void importIntelClicked();

private slots:
    // 内部slot：处理任务项点击事件
    void onTaskItemClicked(QString taskId);

private:
    // 初始化UI界面：创建头部、滚动区域和底部按钮
    void setupUi();

    // 更新任务数量徽章：根据当前任务数量更新显示
    void updateBadge();

    QVBoxLayout *m_mainLayout;            // 主垂直布局
    QFrame *m_headerFrame;                // 列表头部框架（包含标题和徽章）
    QLabel *m_badgeLabel;                 // 任务数量徽章标签
    QScrollArea *m_scrollArea;            // 滚动区域（包含任务列表）
    QWidget *m_scrollContent;             // 滚动内容容器
    QVBoxLayout *m_taskListLayout;         // 任务列表垂直布局

    QMap<QString, TaskItemWidget*> m_taskItems;  // 任务项映射表（key为任务ID）
    QString m_selectedTaskId;              // 当前选中的任务ID
};

#endif // TASKLISTWIDGET_H
