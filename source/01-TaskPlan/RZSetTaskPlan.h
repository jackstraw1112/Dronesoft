// 任务创建/编辑页头文件
// 负责任务基本信息录入、点目标与区域目标的增删改，并对外发出保存信号。
// 该组件作为步骤1页面使用，支持“新建任务”和“加载任务编辑”两种模式。

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
    // 构造/析构
    explicit RZSetTaskPlan(QWidget *parent = nullptr);
    ~RZSetTaskPlan() override;

    // 触发保存流程（可控制是否弹出成功提示）
    bool triggerSave(bool showSuccessMessage = true);
    // 重置界面到“新建任务”状态
    void resetForNewTask();
    // 加载已有任务到界面（用于编辑）
    void loadTaskForEdit(const TaskBasicInfo &taskInfo, const QList<PointTargetInfo> &pointTargets, const QList<AreaTargetInfo> &areaTargets);

    // 获取当前点目标数量（用于实时同步任务列表计数）
    int pointTargetCount() const { return m_pointTargets.size(); }
    // 获取当前区域目标数量（用于实时同步任务列表计数）
    int areaTargetCount() const { return m_areaTargets.size(); }
    // 获取当前点目标列表（用于同步内存缓存）
    const QList<PointTargetInfo> &pointTargets() const { return m_pointTargets; }
    // 获取当前区域目标列表（用于同步内存缓存）
    const QList<AreaTargetInfo> &areaTargets() const { return m_areaTargets; }

signals:
    // 保存任务时发射，返回当前界面填写的任务基本信息
    void saveTaskClicked(const TaskBasicInfo &taskInfo);
    // 保存任务时发射，返回任务基本信息和点目标列表
    void saveTaskWithPointTargetsClicked(const TaskBasicInfo &taskInfo, const QList<PointTargetInfo> &pointTargets);
    // 保存任务时发射完整任务数据（用于缓存更新）
    void saveTaskDetailClicked(const TaskPlanningData &taskData);
    // 点目标/区域目标增删时发射，用于实时同步任务列表卡片中的目标数量
    void targetsModified();

private:
    // 初始化参数
    void initParams();
    // 初始化对象
    void initObject();
    // 关联信号与槽函数
    void initConnect();

    // 保存任务
    void onSaveTask();

    // 点目标：添加
    void onAddPointTarget();
    // 点目标：编辑
    void onEditPointTarget();
    // 点目标：删除
    void onDeletePointTarget();
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

    // 区域目标：添加
    void onAddAreaTarget();
    // 区域目标：编辑
    void onEditAreaTarget();
    // 区域目标：删除
    void onDeleteAreaTarget();
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
    // 最近一次保存或加载的任务基础信息（用于编辑时保留 taskUid）。
    TaskBasicInfo m_lastSavedTaskInfo;
    // 点目标内存缓存（作为 UI 表格的数据源）。
    QList<PointTargetInfo> m_pointTargets;
    // 区域目标内存缓存（作为 UI 表格的数据源）。
    QList<AreaTargetInfo> m_areaTargets;
    // 页面是否处于可编辑状态（新建/编辑=true，查看=false）。
    bool m_formEditable = false;
};

#endif // RZSIM_ANTI_RADIATION_UAV_ADDOREDITTASKPLAN_H
