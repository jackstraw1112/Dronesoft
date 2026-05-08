//
// Created by Administrator on 2026/5/7.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H
#define RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H

#include <QFrame>
#include <QList>
#include <QString>
#include "StructData.h"
#include "PathAlgorithm/path_planner.h"


QT_BEGIN_NAMESPACE
namespace Ui { class RoutePlanning; }
QT_END_NAMESPACE

// 无人机任务分配：描述每架无人机被分配的目标信息
struct UavAssignment {
    QString uavId;          // 无人机编号（如 "UAV-01"）
    int uavIndex = 0;       // 全局索引（0-based）
    QString targetId;       // 目标编号（如 "PT-01"）
    QString targetName;     // 目标名称（如 "东郊制导雷达"）
    QString targetType;     // 目标类型（"PT" / "AR"）
};


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

private:
    void applyTechStyle();

    // 根据 UI 参数和指定无人机分组构建规划输入
    PlanningInput buildPlanningInput(const QList<UavAssignment> &group,
                                     const std::vector<GeoPoint> &startPositions,
                                     const std::vector<GeoPoint> &targetArea);

    // 将规划结果填入表格
    void populateTable(const PlanningResult &result, const QString &targetName);

    // 解析 UI 中的字符串数值
    static double parseValue(const QString &text);
    static double parseSpeed(const QString &text);

    Ui::RoutePlanning *ui;
    QList<UavAssignment> m_assignments;
    int m_totalUavCount = 0;
};


#endif //RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H
