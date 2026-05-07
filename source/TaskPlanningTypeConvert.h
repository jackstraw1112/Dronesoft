#ifndef TASKPLANNINGTYPECONVERT_H
#define TASKPLANNINGTYPECONVERT_H

#include <QString>
#include "TaskPlanningData.h"

// ===================== 点目标类型转换接口 =====================

inline QString targetTypeToChinese(TargetType type)
{
    switch (type)
    {
    case TargetType::EarlyWarningRadar:  return QStringLiteral("预警雷达");
    case TargetType::SurveillanceRadar:  return QStringLiteral("监视雷达");
    case TargetType::TrackingRadar:      return QStringLiteral("跟踪雷达");
    case TargetType::FireControlRadar:   return QStringLiteral("火控雷达");
    case TargetType::HeightFindingRadar: return QStringLiteral("测高雷达");
    case TargetType::MultiFunctionRadar: return QStringLiteral("多功能相控阵雷达");
    case TargetType::GuidanceRadar:      return QStringLiteral("制导雷达");
    case TargetType::CommStation:        return QStringLiteral("通信/指挥台");
    case TargetType::DataLinkNode:       return QStringLiteral("数传节点");
    case TargetType::Jammer:             return QStringLiteral("干扰设备");
    case TargetType::NavAid:             return QStringLiteral("导航/标信台");
    default:                             return QStringLiteral("其他");
    }
}

inline QString targetTypeToEnglish(TargetType type)
{
    switch (type)
    {
    case TargetType::EarlyWarningRadar:  return QStringLiteral("Early Warning Radar");
    case TargetType::SurveillanceRadar:  return QStringLiteral("Surveillance Radar");
    case TargetType::TrackingRadar:      return QStringLiteral("Tracking Radar");
    case TargetType::FireControlRadar:   return QStringLiteral("Fire Control Radar");
    case TargetType::HeightFindingRadar: return QStringLiteral("Height-Finding Radar");
    case TargetType::MultiFunctionRadar: return QStringLiteral("Multi-Function Radar");
    case TargetType::GuidanceRadar:      return QStringLiteral("Guidance Radar");
    case TargetType::CommStation:        return QStringLiteral("Comm/Command Station");
    case TargetType::DataLinkNode:       return QStringLiteral("Data Link Node");
    case TargetType::Jammer:             return QStringLiteral("Jammer");
    case TargetType::NavAid:             return QStringLiteral("Navigation Aid");
    default:                             return QStringLiteral("Unknown");
    }
}

inline TargetType targetTypeFromChinese(const QString &cn)
{
    if (cn.contains(QStringLiteral("预警")))      return TargetType::EarlyWarningRadar;
    if (cn.contains(QStringLiteral("监视")))      return TargetType::SurveillanceRadar;
    if (cn.contains(QStringLiteral("跟踪")))      return TargetType::TrackingRadar;
    if (cn.contains(QStringLiteral("火控")))      return TargetType::FireControlRadar;
    if (cn.contains(QStringLiteral("测高")))      return TargetType::HeightFindingRadar;
    if (cn.contains(QStringLiteral("相控阵")) ||
        cn.contains(QStringLiteral("多功能")))    return TargetType::MultiFunctionRadar;
    if (cn.contains(QStringLiteral("制导")))      return TargetType::GuidanceRadar;
    if (cn.contains(QStringLiteral("通信")) ||
        cn.contains(QStringLiteral("指挥")))      return TargetType::CommStation;
    if (cn.contains(QStringLiteral("数传")) ||
        cn.contains(QStringLiteral("数据链")))    return TargetType::DataLinkNode;
    if (cn.contains(QStringLiteral("干扰")))      return TargetType::Jammer;
    if (cn.contains(QStringLiteral("导航")) ||
        cn.contains(QStringLiteral("标信")))      return TargetType::NavAid;
    return TargetType::Unknown;
}

inline TargetType targetTypeFromEnglish(const QString &en)
{
    const QString t = en.trimmed().toLower();

    if (t.contains("early") && t.contains("warning"))  return TargetType::EarlyWarningRadar;
    if (t.contains("surveillance"))                    return TargetType::SurveillanceRadar;
    if (t.contains("tracking"))                        return TargetType::TrackingRadar;
    if (t.contains("fire control"))                    return TargetType::FireControlRadar;
    if (t.contains("height") && t.contains("finding")) return TargetType::HeightFindingRadar;
    if (t.contains("multi") && t.contains("function")) return TargetType::MultiFunctionRadar;
    if (t.contains("guidance"))                        return TargetType::GuidanceRadar;
    if (t.contains("comm") || t.contains("command"))   return TargetType::CommStation;
    if (t.contains("data") && t.contains("link"))      return TargetType::DataLinkNode;
    if (t.contains("jammer"))                          return TargetType::Jammer;
    if (t.contains("navigation") || t.contains("nav")) return TargetType::NavAid;

    return TargetType::Unknown;
}

// ===================== 任务类型转换 =====================

inline QString taskTypeToChinese(TaskType t)
{
    switch (t)
    {
    case TaskType::SEAD:   return QStringLiteral("SEAD · 防空压制");
    case TaskType::DEAD:   return QStringLiteral("DEAD · 防空摧毁");
    case TaskType::SEARCH: return QStringLiteral("SEARCH · 游猎扫荡");
    default:               return QStringLiteral("未知类型");
    }
}

inline QString taskTypeToEnglish(TaskType t)
{
    switch (t)
    {
    case TaskType::SEAD:   return QStringLiteral("SEAD");
    case TaskType::DEAD:   return QStringLiteral("DEAD");
    case TaskType::SEARCH: return QStringLiteral("SEARCH");
    default:               return QStringLiteral("UNKNOWN");
    }
}

inline TaskType taskTypeFromChinese(const QString &cn)
{
    if (cn.contains(QStringLiteral("SEAD")))   return TaskType::SEAD;
    if (cn.contains(QStringLiteral("DEAD")))   return TaskType::DEAD;
    if (cn.contains(QStringLiteral("SEARCH"))) return TaskType::SEARCH;
    return TaskType::Unknown;
}

inline TaskType taskTypeFromEnglish(const QString &en)
{
    const QString t = en.trimmed().toUpper();
    if (t.startsWith("SEAD"))   return TaskType::SEAD;
    if (t.startsWith("DEAD"))   return TaskType::DEAD;
    if (t.startsWith("SEARCH")) return TaskType::SEARCH;
    return TaskType::Unknown;
}

// ===================== 任务优先级转换 =====================

inline QString priorityToChinese(PriorityLevel p)
{
    switch (p)
    {
    case PriorityLevel::P1: return QStringLiteral("P1 · 紧急");
    case PriorityLevel::P2: return QStringLiteral("P2 · 高");
    case PriorityLevel::P3: return QStringLiteral("P3 · 一般");
    default:                return QStringLiteral("未知");
    }
}

inline QString priorityToEnglish(PriorityLevel p)
{
    switch (p)
    {
    case PriorityLevel::P1: return QStringLiteral("P1");
    case PriorityLevel::P2: return QStringLiteral("P2");
    case PriorityLevel::P3: return QStringLiteral("P3");
    default:                return QStringLiteral("UNKNOWN");
    }
}

inline PriorityLevel priorityFromChinese(const QString &cn)
{
    if (cn.startsWith(QStringLiteral("P1"))) return PriorityLevel::P1;
    if (cn.startsWith(QStringLiteral("P2"))) return PriorityLevel::P2;
    if (cn.startsWith(QStringLiteral("P3"))) return PriorityLevel::P3;
    return PriorityLevel::Unknown;
}

inline PriorityLevel priorityFromEnglish(const QString &en)
{
    const QString t = en.trimmed().toUpper();
    if (t.startsWith("P1")) return PriorityLevel::P1;
    if (t.startsWith("P2")) return PriorityLevel::P2;
    if (t.startsWith("P3")) return PriorityLevel::P3;
    return PriorityLevel::Unknown;
}

// ===================== 威胁等级转换 =====================

inline QString threatLevelTypeToChinese(ThreatLevelType t)
{
    switch (t)
    {
    case ThreatLevelType::High:   return QStringLiteral("高");
    case ThreatLevelType::Medium: return QStringLiteral("中");
    case ThreatLevelType::Low:    return QStringLiteral("低");
    default:                      return QStringLiteral("未知");
    }
}

inline QString threatLevelTypeToEnglish(ThreatLevelType t)
{
    switch (t)
    {
    case ThreatLevelType::High:   return QStringLiteral("HIGH");
    case ThreatLevelType::Medium: return QStringLiteral("MEDIUM");
    case ThreatLevelType::Low:    return QStringLiteral("LOW");
    default:                      return QStringLiteral("UNKNOWN");
    }
}

inline ThreatLevelType threatLevelTypeFromChinese(const QString &cn)
{
    if (cn.contains(QStringLiteral("高"))) return ThreatLevelType::High;
    if (cn.contains(QStringLiteral("中"))) return ThreatLevelType::Medium;
    if (cn.contains(QStringLiteral("低"))) return ThreatLevelType::Low;
    return ThreatLevelType::Unknown;
}

inline ThreatLevelType threatLevelTypeFromEnglish(const QString &en)
{
    const QString t = en.trimmed().toUpper();
    if (t.startsWith("HIGH") || t.startsWith("H")) return ThreatLevelType::High;
    if (t.startsWith("MED")  || t.startsWith("M")) return ThreatLevelType::Medium;
    if (t.startsWith("LOW")  || t.startsWith("L")) return ThreatLevelType::Low;
    return ThreatLevelType::Unknown;
}

// ===================== 区域几何类型转换 =====================

inline QString areaGeometryTypeToChinese(AreaGeometryType g)
{
    switch (g)
    {
    case AreaGeometryType::Circle:    return QStringLiteral("圆形");
    case AreaGeometryType::Rectangle: return QStringLiteral("矩形");
    case AreaGeometryType::Polygon:   return QStringLiteral("多边形");
    case AreaGeometryType::Corridor:  return QStringLiteral("走廊");
    default:                          return QStringLiteral("其他");
    }
}

inline QString areaGeometryTypeToEnglish(AreaGeometryType g)
{
    switch (g)
    {
    case AreaGeometryType::Circle:    return QStringLiteral("Circle");
    case AreaGeometryType::Rectangle: return QStringLiteral("Rectangle");
    case AreaGeometryType::Polygon:   return QStringLiteral("Polygon");
    case AreaGeometryType::Corridor:  return QStringLiteral("Corridor");
    default:                          return QStringLiteral("Unknown");
    }
}

inline AreaGeometryType areaGeometryTypeFromChinese(const QString &cn)
{
    if (cn.contains(QStringLiteral("圆")))    return AreaGeometryType::Circle;
    if (cn.contains(QStringLiteral("矩形")))  return AreaGeometryType::Rectangle;
    if (cn.contains(QStringLiteral("多边形")))return AreaGeometryType::Polygon;
    if (cn.contains(QStringLiteral("走廊")) ||
        cn.contains(QStringLiteral("走廊带"))||
        cn.contains(QStringLiteral("走廊区")))return AreaGeometryType::Corridor;
    return AreaGeometryType::Unknown;
}

inline AreaGeometryType areaGeometryTypeFromEnglish(const QString &en)
{
    const QString t = en.trimmed().toLower();
    if (t.contains("circle"))    return AreaGeometryType::Circle;
    if (t.contains("rect"))      return AreaGeometryType::Rectangle;
    if (t.contains("polygon"))   return AreaGeometryType::Polygon;
    if (t.contains("corridor") ||
        t.contains("band") ||
        t.contains("strip"))     return AreaGeometryType::Corridor;
    return AreaGeometryType::Unknown;
}

// ===================== 搜索策略类型转换 =====================

inline QString searchStrategyTypeToChinese(SearchStrategyType s)
{
    switch (s)
    {
    case SearchStrategyType::SpiralScan:    return QStringLiteral("螺旋扫描");
    case SearchStrategyType::ParallelLines: return QStringLiteral("平行线扫描");
    case SearchStrategyType::SectorScan:    return QStringLiteral("扇形扫描");
    case SearchStrategyType::Loiter:        return QStringLiteral("盘旋搜索");
    default:                                return QStringLiteral("其他");
    }
}

inline QString searchStrategyTypeToEnglish(SearchStrategyType s)
{
    switch (s)
    {
    case SearchStrategyType::SpiralScan:    return QStringLiteral("Spiral Scan");
    case SearchStrategyType::ParallelLines: return QStringLiteral("Parallel Lines");
    case SearchStrategyType::SectorScan:    return QStringLiteral("Sector Scan");
    case SearchStrategyType::Loiter:        return QStringLiteral("Loiter");
    default:                                return QStringLiteral("Unknown");
    }
}

inline SearchStrategyType searchStrategyTypeFromChinese(const QString &cn)
{
    if (cn.contains(QStringLiteral("螺旋")))   return SearchStrategyType::SpiralScan;
    if (cn.contains(QStringLiteral("平行线"))) return SearchStrategyType::ParallelLines;
    if (cn.contains(QStringLiteral("扇形")))   return SearchStrategyType::SectorScan;
    if (cn.contains(QStringLiteral("盘旋")))   return SearchStrategyType::Loiter;
    return SearchStrategyType::Unknown;
}

inline SearchStrategyType searchStrategyTypeFromEnglish(const QString &en)
{
    const QString t = en.trimmed().toLower();
    if (t.contains("spiral"))    return SearchStrategyType::SpiralScan;
    if (t.contains("parallel"))  return SearchStrategyType::ParallelLines;
    if (t.contains("sector"))    return SearchStrategyType::SectorScan;
    if (t.contains("loiter"))    return SearchStrategyType::Loiter;
    return SearchStrategyType::Unknown;
}

// ===================== 方案阶段转换 =====================

inline QString taskPlanStageToChinese(TaskPlanStage s)
{
    switch (s)
    {
    case TaskPlanStage::Scheme:   return QStringLiteral("方案阶段");
    case TaskPlanStage::Editing:  return QStringLiteral("编辑阶段");
    case TaskPlanStage::Simulate: return QStringLiteral("仿真阶段");
    default:                      return QStringLiteral("未知阶段");
    }
}

inline TaskPlanStage taskPlanStageFromChinese(const QString &cn)
{
    if (cn.contains(QStringLiteral("方案"))) return TaskPlanStage::Scheme;
    if (cn.contains(QStringLiteral("编辑"))) return TaskPlanStage::Editing;
    if (cn.contains(QStringLiteral("仿真"))) return TaskPlanStage::Simulate;
    return TaskPlanStage::Unknown;
}

// ===================== 任务状态转换 =====================

inline QString taskStatusTypeToChinese(TaskStatusType s)
{
    switch (s)
    {
    case TaskStatusType::Pending:  return QStringLiteral("待执行");
    case TaskStatusType::Planning: return QStringLiteral("规划中");
    case TaskStatusType::Running:  return QStringLiteral("执行中");
    default:                       return QStringLiteral("未知");
    }
}

inline TaskStatusType taskStatusTypeFromChinese(const QString &cn)
{
    if (cn.contains(QStringLiteral("待执行"))) return TaskStatusType::Pending;
    if (cn.contains(QStringLiteral("规划中"))) return TaskStatusType::Planning;
    if (cn.contains(QStringLiteral("执行中"))) return TaskStatusType::Running;
    return TaskStatusType::Unknown;
}

#endif // TASKPLANNINGTYPECONVERT_H

