// 任务列表组件实现文件
// ---------------------------------------------------------------------------
// 职责说明：
// 1) 承载任务卡片（TaskItemWidget）的创建、销毁与排布；
// 2) 维护“当前选中任务”的状态，并向外发射选中/删除信号；
// 3) 在容器尺寸变化时动态收敛卡片最大宽度，保证视觉居中与布局稳定。
//
// 设计要点：
// - 任务项以 taskId 为唯一键，保存在 m_taskItems 中，避免重复添加；
// - 选中状态统一由 setSelectedTask() 管理，防止多处直接改状态导致不一致；
// - 删除任务后会自动补选一个任务，保证外部页面仍有明确上下文（若列表非空）。

#include "RZTaskListWidget.h"
#include <QDebug>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include "ui_RZTaskListWidget.h"

// 构造函数：完成 UI 建立、内部状态初始化、信号槽绑定
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

// 初始化参数：仅处理轻量状态，不涉及 QWidget 行为
void RZTaskListWidget::initParams()
{
    m_selectedTaskId.clear();
}

// 初始化对象：缓存布局指针并同步一次宽度约束与徽章
void RZTaskListWidget::initObject()
{
    // scrollContent 在 .ui 中挂载了垂直布局，用于顺序放置任务卡片
    m_taskListLayout = qobject_cast<QVBoxLayout *>(ui->scrollContent->layout());
    updateTaskItemWidthConstraints();
    updateBadge();
}

// 关联信号与槽函数：将顶部按钮动作转为组件对外语义
void RZTaskListWidget::initConnect()
{
    connect(ui->newTaskBtn, &QPushButton::clicked, this, &RZTaskListWidget::newTaskClicked);
    connect(ui->deleteTaskBtn, &QPushButton::clicked, this, &RZTaskListWidget::onDeleteTaskClicked);
}

// 添加任务项：根据传入字段创建任务卡片并插入列表
// 参数语义：
// - taskId/taskName/status/time：直接映射到卡片显示；
// - targets：目标数量；
// - aircraft：用于推导“威胁等级”展示（高/中/低）。
void RZTaskListWidget::addTask(QString taskId, QString taskName,
                               QString status, int targets, int aircraft,
                               QString time)
{
    // 检查是否已存在相同ID的任务，避免重复添加
    if (m_taskItems.contains(taskId))
    {
        return;
    }

    // 创建任务项组件（父对象设为 scrollContent，便于布局和生命周期统一管理）
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

    // 卡片点击 -> 统一进入列表的选中流程
    connect(item, &TaskItemWidget::clicked, this, &RZTaskListWidget::onTaskItemClicked);

    // 在底部弹性空间之前插入任务项，保持“任务在上、空白在下”的滚动体验
    m_taskListLayout->insertWidget(m_taskListLayout->count() - 1, item);
    m_taskListLayout->setAlignment(item, Qt::AlignHCenter);
    // 注册到映射表，后续选中/删除/遍历都基于该容器
    m_taskItems.insert(taskId, item);
    updateTaskItemWidthConstraints();

    // 更新徽章显示
    updateBadge();

    // 首次添加任务时自动选中，避免“列表有内容但右侧无上下文”的状态
    if (m_selectedTaskId.isEmpty())
    {
        setSelectedTask(taskId);
    }
}

// 清空所有任务项：释放所有卡片并重置内部状态
// 注意：此处直接 delete 子项，确保立即释放资源；随后清空索引容器。
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

// 设置选中的任务：更新高亮并通知外部“当前任务已切换”
// 该函数是唯一合法的“选中状态切换入口”。
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

// 更新任务数量徽章：当前版本 badgeLabel 已移除，保留空实现作兼容扩展点
void RZTaskListWidget::updateBadge()
{
    // badgeLabel 已移除，此处保留为空实现，便于后续扩展。
}

// 内部槽：任务卡片被点击时，转发到统一选中逻辑
void RZTaskListWidget::onTaskItemClicked(QString taskId)
{
    setSelectedTask(taskId);
}

// 删除当前选中的任务：
// 1) 先从映射表摘除并清理选中态；
// 2) 删除控件并更新布局约束；
// 3) 通知外部任务被删除；
// 4) 若仍有剩余任务，自动选中第一个任务作为新的上下文。
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

// 统一收敛任务卡片最大宽度，避免以下问题：
// - 父容器变窄后卡片仍保持旧宽度，造成视觉“偏移”；
// - 卡片宽于 viewport，出现不必要的水平拥挤/错位感。
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

// 添加测试任务数据：用于本地联调/界面演示
// 覆盖“执行中/规划中/待执行”三种常见状态。
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
