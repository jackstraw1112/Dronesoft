// 任务列表组件头文件
// 任务列表组件用于显示和管理任务项，支持任务的添加、选择和删除操作
// 每个任务项包含任务ID、名称、状态、目标数、架次数和时间等基本信息

#ifndef TASKLISTWIDGET_H
#define TASKLISTWIDGET_H

// Qt核心组件
#include <QMap>
#include <QWidget>

#include "TaskItemWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class RZTaskListWidget;
}
QT_END_NAMESPACE

class QVBoxLayout;
class QResizeEvent;

// 任务列表组件类：管理多个任务项的容器组件
// 提供任务列表的滚动显示、新建任务和删除任务功能
class RZTaskListWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数：创建任务列表组件，初始化UI布局
    explicit RZTaskListWidget(QWidget *parent = nullptr);
    ~RZTaskListWidget();

    // 添加任务项：根据提供的信息创建并显示任务卡片
    // 参数：任务ID、名称、状态、目标数量、飞机架次、时间
    void addTask(QString taskId, QString taskName, QString status, int targets, int aircraft, QString time);

    // 清空所有任务项：删除所有任务卡片并重置选中状态
    void clearTasks();

    // 设置选中的任务：高亮显示指定任务并发射选中信号
    void setSelectedTask(QString taskId);

    // 获取当前任务数量
    int getTaskCount()
    {
        return m_taskItems.size();
    }

    // 添加测试任务数据：用于演示任务列表效果
    void addTestTasks();

signals:
    // 任务被选中时发射的信号，包含任务ID
    void taskSelected(QString taskId);

    // 新建任务按钮被点击时发射的信号
    void newTaskClicked();

    // 删除任务按钮被点击时发射的信号（参数为被删除任务ID）
    void deleteTaskClicked(QString taskId);

private slots:
    // 内部slot：处理任务项点击事件
    void onTaskItemClicked(QString taskId);
    // 删除当前选中任务
    void onDeleteTaskClicked();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // 初始化参数
    void initParams();
    // 初始化对象
    void initObject();
    // 关联信号与槽函数
    void initConnect();

    // 更新任务数量徽章：根据当前任务数量更新显示
    void updateBadge();
    // 根据滚动区域可用宽度刷新任务卡片宽度约束
    void updateTaskItemWidthConstraints();

private:
    Ui::RZTaskListWidget *ui;

    // 任务列表垂直布局
    QVBoxLayout *m_taskListLayout = nullptr;

    // 任务项映射表（key为任务ID）
    QMap<QString, TaskItemWidget *> m_taskItems;

    // 当前选中的任务ID
    QString m_selectedTaskId;
};

#endif // TASKLISTWIDGET_H
