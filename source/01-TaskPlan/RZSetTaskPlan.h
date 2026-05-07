//
// Created by Administrator on 2026/5/6.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_ADDOREDITTASKPLAN_H
#define RZSIM_ANTI_RADIATION_UAV_ADDOREDITTASKPLAN_H

#include <QWidget>
#include "../TaskPlanningData.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class RZSetTaskPlan;
}
QT_END_NAMESPACE

class RZSetTaskPlan : public QWidget
{
    Q_OBJECT

public:
    explicit RZSetTaskPlan(QWidget *parent = nullptr);
    ~RZSetTaskPlan() override;

    // 触发保存流程（可控制是否弹出成功提示）
    bool triggerSave(bool showSuccessMessage = true);
    // 重置界面到“新建任务”状态
    void resetForNewTask();
    // 加载已有任务到界面（用于编辑）
    void loadTaskForEdit(const TaskBasicInfo &taskInfo,
                         const QList<PointTargetInfo> &pointTargets,
                         const QList<AreaTargetInfo> &areaTargets);

signals:
    // 保存任务时发射，返回当前界面填写的任务基本信息
    void saveTaskClicked(const TaskBasicInfo &taskInfo);
    // 保存任务时发射，返回任务基本信息和点目标列表
    void saveTaskWithPointTargetsClicked(const TaskBasicInfo &taskInfo, const QList<PointTargetInfo> &pointTargets);
    // 保存任务时发射完整任务数据（用于缓存更新）
    void saveTaskDetailClicked(const TaskPlanningData &taskData);

private:
    // 初始化参数
    void initParams();
    // 初始化对象
    void initObject();
    // 关联信号与槽函数
    void initConnect();

    // 保存任务按钮槽函数
    void onSaveTaskBtnClicked();
    // 添加点目标
    void onAddPointTargetBtnClicked();
    // 编辑点目标
    void onEditPointTargetBtnClicked();
    // 删除点目标
    void onDeletePointTargetBtnClicked();
    // 双击编辑点目标
    void onPointTargetDoubleClicked(int row, int column);
    // 点目标表格右键菜单
    void onPointTargetContextMenu(const QPoint &pos);
    // 编辑指定行点目标（可用于双击/右键菜单复用）
    void editPointTargetAtRow(int row);
    // 刷新点目标编辑/删除按钮可用状态
    void updatePointTargetActionBtnState();
    // 将点目标写入指定表格行
    void setPointTargetRow(int row, const PointTargetInfo &targetInfo);
    // 添加区域目标
    void onAddAreaTargetBtnClicked();
    // 编辑区域目标
    void onEditAreaTargetBtnClicked();
    // 删除区域目标
    void onDeleteAreaTargetBtnClicked();
    // 刷新区域目标编辑/删除按钮可用状态
    void updateAreaTargetActionBtnState();
    // 将区域目标写入指定表格行
    void setAreaTargetRow(int row, const AreaTargetInfo &targetInfo);
    // 编辑指定行区域目标
    void editAreaTargetAtRow(int row);
    // 强制刷新界面显示（用于新增后立即可见）
    void forceRefreshView();
    // 设置界面可编辑状态（添加/编辑时开启）
    void setFormEditable(bool editable);
    // 是否允许操作
    bool canOperate() const;

private:
    Ui::RZSetTaskPlan *ui;
    TaskBasicInfo m_lastSavedTaskInfo;
    QList<PointTargetInfo> m_pointTargets;
    QList<AreaTargetInfo> m_areaTargets;
    bool m_formEditable = false;
};

#endif // RZSIM_ANTI_RADIATION_UAV_ADDOREDITTASKPLAN_H
