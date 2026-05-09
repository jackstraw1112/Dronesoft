//
// Created by Administrator on 2026/5/7.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H
#define RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H

#include <QFrame>
#include <QList>
#include <QString>
#include "TaskPlanningData.h"
#include "PathAlgorithm/path_planner.h"
#include "PathDisplayDialog.h"


QT_BEGIN_NAMESPACE
namespace Ui { class RoutePlanning; }
QT_END_NAMESPACE


class RoutePlanning : public QFrame {
Q_OBJECT

public:
    explicit RoutePlanning(QWidget *parent = nullptr);

    ~RoutePlanning() override;

    // 设置协同任务分配的无人机编组数据（由 MissionPlanner 在步骤切换时调用）
    void setAllocationData(const QList<UavAssignment> &assignments, int totalUavCount);

private slots:
    // "一键自动规划"按钮点击
    void onPlanAllClicked();

    // 逐组处理规划
    void processNextGroup();

    // 逐行追加表格内容（延时）
    void appendNextRow();

private:
    void applyTechStyle();

    // 根据 UI 参数和指定无人机分组构建规划输入
    PlanningInput buildPlanningInput(const QList<UavAssignment> &group,
                                     const std::vector<GeoPoint> &startPositions,
                                     const std::vector<GeoPoint> &targetArea);

    // 解析 UI 中的字符串数值
    static double parseValue(const QString &text);
    static double parseSpeed(const QString &text);

    // 分组规划数据（预计算）
    struct GroupPlanData {
        PlanningInput input;
        QString targetName;
        int uavCount = 0;
    };

    // 分组规划结果
    struct GroupPlanResult {
        int uavCount = 0;
        PlanningResult result;
        QString targetName;
    };

    Ui::RoutePlanning *ui;
    QList<UavAssignment> m_assignments;
    int m_totalUavCount = 0;

    // 延时逐行填充状态
    QList<GroupPlanData> m_pendingGroups;
    QList<GroupPlanResult> m_planningResults;
    int m_currentGroupIndex = 0;

    int m_currentRowInGroup = 0;
    int m_groupStartRow = 0;
    PlanningResult m_currentGroupResult;
    QString m_currentGroupTargetName;
    int m_currentGroupUavCount = 0;

    // 每行航迹数据（用于查看详情弹窗）
    QList<QPair<UAVPath, UAVPath>> m_rowPaths;
};


#endif //RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H
