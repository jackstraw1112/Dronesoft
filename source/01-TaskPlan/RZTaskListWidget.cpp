// 任务列表组件实现文件
// 实现任务列表组件的功能

#include "RZTaskListWidget.h"
#include <QDebug>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include "ui_RZTaskListWidget.h"

// 任务列表组件构造函数
RZTaskListWidget::RZTaskListWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::RZTaskListWidget)
{
    ui->setupUi(this);

    // 初始化参数
    initParams();

    // 初始化对象
    initObject();

    // 关联信号与槽函数
    initConnect();
}

RZTaskListWidget::~RZTaskListWidget()
{
    delete ui;
}

// 初始化参数
void RZTaskListWidget::initParams()
{
    m_selectedTaskId.clear();
}

// 初始化对象
void RZTaskListWidget::initObject()
{
    m_taskListLayout = qobject_cast<QVBoxLayout *>(ui->scrollContent->layout());
    updateTaskItemWidthConstraints();
    updateBadge();
}

// 关联信号与槽函数
void RZTaskListWidget::initConnect()
{
    connect(ui->newTaskBtn, &QPushButton::clicked, this, &RZTaskListWidget::newTaskClicked);
    connect(ui->deleteTaskBtn, &QPushButton::clicked, this, &RZTaskListWidget::onDeleteTaskClicked);
}

// 添加任务项：根据提供的信息创建并显示任务卡片
void RZTaskListWidget::addTask(QString taskId, QString taskName,
                               QString status, int targets, int aircraft,
                               QString time)
{
    // 检查是否已存在相同ID的任务，避免重复添加
    if (m_taskItems.contains(taskId))
    {
        return;
    }

    // 创建任务项组件
    TaskItemWidget *item = new TaskItemWidget(ui->scrollContent);

    // 任务卡片横向不拉伸，占满可用宽度上限后由布局居中
    item->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    item->setTaskId(taskId);
    item->setTaskName(taskName);
    item->setTaskStatus(status);
    item->setTargetCount(targets);
    item->setTime(time);
    const QString threatLevel = (aircraft >= 10) ? QStringLiteral("高")
            : (aircraft >= 6)                    ? QStringLiteral("中")
                                                 : QStringLiteral("低");
    item->setThreatLevel(threatLevel);

    // 连接任务项点击信号到列表的槽函数
    connect(item, &TaskItemWidget::clicked, this, &RZTaskListWidget::onTaskItemClicked);

    // 在弹性空间之前插入任务项
    m_taskListLayout->insertWidget(m_taskListLayout->count() - 1, item);
    m_taskListLayout->setAlignment(item, Qt::AlignHCenter);
    // 添加到任务映射表
    m_taskItems.insert(taskId, item);
    updateTaskItemWidthConstraints();

    // 更新徽章显示
    updateBadge();

    // 如果没有选中的任务，自动选中新添加的任务
    if (m_selectedTaskId.isEmpty())
    {
        setSelectedTask(taskId);
    }
}

// 清空所有任务项：删除所有任务卡片并重置选中状态
void RZTaskListWidget::clearTasks()
{
    // 遍历映射表，删除所有任务项组件
    QMap<QString, TaskItemWidget *>::iterator it = m_taskItems.begin();
    while (it != m_taskItems.end())
    {
        delete it.value();
        ++it;
    }
    m_taskItems.clear();      // 清空映射表
    m_selectedTaskId.clear(); // 清空选中状态
    updateTaskItemWidthConstraints();
    updateBadge(); // 更新徽章
}

// 设置选中的任务：高亮显示指定任务并发射选中信号
void RZTaskListWidget::setSelectedTask(QString taskId)
{
    // 检查任务是否存在
    if (!m_taskItems.contains(taskId))
    {
        return;
    }

    // 取消上一个选中任务的选中状态
    if (!m_selectedTaskId.isEmpty() && m_taskItems.contains(m_selectedTaskId))
    {
        m_taskItems.value(m_selectedTaskId)->setSelected(false);
    }

    // 设置新的选中状态
    m_selectedTaskId = taskId;
    m_taskItems.value(taskId)->setSelected(true);
    emit taskSelected(taskId); // 发射选中信号
}

// 更新任务数量徽章：根据当前任务数量更新显示
void RZTaskListWidget::updateBadge()
{
    // badgeLabel 已移除，此处保留为空实现，便于后续扩展。
}

// 内部slot：处理任务项点击事件
void RZTaskListWidget::onTaskItemClicked(QString taskId)
{
    setSelectedTask(taskId);
}

void RZTaskListWidget::onDeleteTaskClicked()
{
    if (m_selectedTaskId.isEmpty() || !m_taskItems.contains(m_selectedTaskId))
    {
        return;
    }

    const QString deletedTaskId = m_selectedTaskId;
    TaskItemWidget *item = m_taskItems.take(deletedTaskId);
    m_selectedTaskId.clear();

    delete item;
    updateTaskItemWidthConstraints();
    updateBadge();
    emit deleteTaskClicked(deletedTaskId);

    if (!m_taskItems.isEmpty())
    {
        setSelectedTask(m_taskItems.first()->getTaskId());
    }
}

void RZTaskListWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTaskItemWidthConstraints();
}

void RZTaskListWidget::updateTaskItemWidthConstraints()
{
    if (!ui || !ui->taskScrollArea || !ui->scrollContent)
    {
        return;
    }

    // 计算滚动内容区可用宽度，避免任务卡片宽度超过容器导致“看起来不居中”
    const int viewportWidth = ui->taskScrollArea->viewport()->width();
    const QMargins margins = m_taskListLayout ? m_taskListLayout->contentsMargins() : QMargins();
    const int availableWidth = qMax(0, viewportWidth - margins.left() - margins.right());

    QMap<QString, TaskItemWidget *>::const_iterator it = m_taskItems.constBegin();
    while (it != m_taskItems.constEnd())
    {
        TaskItemWidget *item = it.value();
        if (item)
        {
            item->setMaximumWidth(availableWidth);
        }
        ++it;
    }
}

// 添加测试任务数据：用于演示任务列表效果
void RZTaskListWidget::addTestTasks()
{
    // 添加5个测试任务，覆盖不同的状态
    addTask("MSN-2026-0501-A", "SEAD预先打击任务 - 雷达站群A区",
            "执行中", 8, 12, "2026-05-03 08:30");

    addTask("MSN-2026-0502-B", "对地攻击任务 - 指挥中心B区",
            "规划中", 5, 8, "2026-05-03 14:00");

    addTask("MSN-2026-0503-C", "压制干扰任务 - 通信中继C区",
            "待执行", 3, 6, "2026-05-04 06:00");
}
