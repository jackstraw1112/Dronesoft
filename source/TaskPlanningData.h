#ifndef TASKPLANNINGDATA_H
#define TASKPLANNINGDATA_H

#include <QDateTime>
#include <QList>
#include <QString>
#include <QUuid>
#include "StructData.h"

// 点目标类型枚举：以雷达为主，兼顾非雷达辐射源
enum class TargetType
{
    Unknown = 0,         // 未知/其他

    // 雷达类
    EarlyWarningRadar,   // 预警雷达（远程搜索）
    SurveillanceRadar,   // 监视/搜索雷达（中近程搜索）
    TrackingRadar,       // 跟踪雷达
    FireControlRadar,    // 火控雷达（制导/照射）
    HeightFindingRadar,  // 测高雷达
    MultiFunctionRadar,  // 多功能相控阵雷达（含搜索+火控）

    // 导引/制导
    GuidanceRadar,       // 制导雷达

    // 非雷达电磁目标
    CommStation,         // 通信/指挥台
    DataLinkNode,        // 数传/数据链节点
    Jammer,              // 干扰/电子战设备
    NavAid               // 导航/标信台（TACAN/VOR 等）
};

// 威胁等级：与界面和 ForceCalculation/TaskAllocation 中的“高/中/低”语义保持一致
enum class ThreatLevelType
{
    Unknown = 0,
    High,    // 高
    Medium,  // 中
    Low      // 低
};



// 任务类型枚举：对应 SEAD / DEAD / SEARCH 等
enum class TaskType
{
    Unknown = 0,
    SEAD,    // 防空压制
    DEAD,    // 防空摧毁
    SEARCH   // 游猎扫荡
};

// 任务优先级枚举：与界面中的 P1/P2/P3 对应
enum class PriorityLevel
{
    Unknown = 0,
    P1,  // 紧急
    P2,  // 高
    P3   // 一般
};

// 任务所处的方案阶段
enum class TaskPlanStage
{
    Unknown = 0,
    Scheme,   // 方案阶段
    Editing,  // 编辑阶段
    Simulate  // 仿真阶段
};

// 任务状态
enum class TaskStatusType
{
    Unknown = 0,
    Pending,    // 待执行
    Planning,   // 规划中
    Running     // 执行中
};

// 点目标：对应界面中“点目标”表格的一行
struct PointTargetInfo
{
    QString         targetId;        // 编号，如 PT-01
    QString         name;            // 目标名称
    TargetType      type;            // 类型（制导雷达、火控雷达等）
    ThreatLevelType threatLevel;     // 威胁等级（高、中、低）
    double          latitude;        // 纬度（度）
    double          longitude;       // 经度（度）
    double          cepMeters;       // 圆概率误差半径（米）
    QString         band;            // 频段（如 I-BAND、H-BAND）
    QString         emissionPattern; // 辐射规律（连续、间歇 等）
    QString         priority;        // 优先级（P1、P2、P3）
    double          requiredPk;      // 毁伤要求 Pk（例如 0.9）

    PointTargetInfo()
        : targetId()
        , name()
        , type(TargetType::Unknown)
        , threatLevel(ThreatLevelType::Unknown)
        , latitude(0.0)
        , longitude(0.0)
        , cepMeters(0.0)
        , band()
        , emissionPattern()
        , priority()
        , requiredPk(0.0)
    {}
};

// 区域目标几何类型
enum class AreaGeometryType
{
    Unknown = 0,  // 未知
    Circle,       // 圆形
    Rectangle,    // 矩形
    Polygon,      // 多边形
    Corridor      // 走廊/带状
};

// 简单经纬度点，用于多边形/走廊等顶点描述
struct GeoPoint
{
    double latitude  = 0.0;
    double longitude = 0.0;
};

// 搜索策略类型（用于区域目标）
enum class SearchStrategyType
{
    Unknown = 0,   // 未知
    SpiralScan,    // 螺旋扫描
    ParallelLines, // 平行线扫描
    SectorScan,    // 扇形扫描
    Loiter,        // 盘旋搜索
};

// 区域目标：对应界面中“区域目标”表格的一行
struct AreaTargetInfo
{
    QString          targetId;         // 编号，如 AR-01
    QString          name;             // 区域名称
    AreaGeometryType areaType;         // 区域类型（圆形、多边形等）
    double           centerLatitude;   // 中心点纬度（度）
    double           centerLongitude;  // 中心点经度（度）
    double           radiusKm;         // 范围半径（千米），圆形/走廊用
    QList<GeoPoint>  vertices;         // 多边形或走廊的顶点列表（地图拾取）
    QString          expectedEmitters; // 预期辐射源描述
    SearchStrategyType searchStrategy; // 搜索策略（螺旋扫描、平行线扫描等）
    PriorityLevel    priority;         // 优先级（P1、P2、P3）

    AreaTargetInfo()
        : targetId()
        , name()
        , areaType(AreaGeometryType::Unknown)
        , centerLatitude(0.0)
        , centerLongitude(0.0)
        , radiusKm(0.0)
        , vertices()
        , expectedEmitters()
        , searchStrategy(SearchStrategyType::Unknown)
        , priority(PriorityLevel::Unknown)
    {}
};

// 按阶段与时间判定任务状态：
// 1) 方案阶段：全部规划中
// 2) 编辑阶段：全部待执行
// 3) 仿真阶段：当前时刻 < 开始时刻 => 待执行，否则执行中
inline TaskStatusType resolveTaskStatus(TaskPlanStage stage,
                                        uint currentTimestampSec,
                                        uint startTimestampSec,
                                        uint endTimestampSec)
{
    Q_UNUSED(endTimestampSec);

    switch (stage)
    {
    case TaskPlanStage::Scheme:
        return TaskStatusType::Planning;
    case TaskPlanStage::Editing:
        return TaskStatusType::Pending;
    case TaskPlanStage::Simulate:
        return (currentTimestampSec < startTimestampSec)
                   ? TaskStatusType::Pending
                   : TaskStatusType::Running;
    default:
        return TaskStatusType::Unknown;
    }
}

// 任务基本信息：对应“1.1 任务基本信息”面板
struct TaskBasicInfo
{
    QString       taskUid;              // 系统任务ID（自动生成，编辑/删除依据）
    QString       taskName;             // 任务名称
    QString       taskId;               // 任务编号
    TaskType      taskType;             // 任务类型（SEAD、DEAD、SEARCH 等）
    PriorityLevel priority;             // 任务优先级
    TaskStatusType status;              // 任务状态（待执行、规划中、执行中）
    uint          startTimestampSec;    // 任务开始时间戳（秒）
    uint          endTimestampSec;      // 任务结束时间戳（秒）
    QString       intent;               // 任务意图

    TaskBasicInfo()
        : taskUid(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , taskName()
        , taskId()
        , taskType(TaskType::Unknown)
        , priority(PriorityLevel::Unknown)
        , status(TaskStatusType::Unknown)
        , startTimestampSec(0)
        , endTimestampSec(0)
        , intent()
    {}
};

/**
 * @brief 任务规划总数据结构体
 */
struct TaskPlanningData
{
    // 单任务核心数据
    TaskBasicInfo basicInfo;             // 任务基本信息
    QList<PointTargetInfo> pointTargets; // 点目标列表
    QList<AreaTargetInfo> areaTargets;   // 区域目标列表

    // 兵力与编组
    QList<ForceCalculation> forceCalcs;  // 兵力需求计算结果
    QList<GroupInfo> groups;             // 编组与装备信息
    QList<TaskAllocation> allocations;   // 任务分配结果

    // 航路规划
    QList<PathPlanning> paths;           // 路径规划结果

    // 目标参数与对抗参数
    QList<RadarTargetParam> radarTargets;  // 雷达目标参数
    QList<RadioTargetParam> radioTargets;  // 电台目标参数
    QList<CommJammingParam> commJamParams; // 通信对抗参数
    QList<RcmJammingParam> rcmJamParams;   // 雷达对抗参数

    TaskPlanningData()
        : basicInfo()
        , pointTargets()
        , areaTargets()
        , forceCalcs()
        , groups()
        , allocations()
        , paths()
        , radarTargets()
        , radioTargets()
        , commJamParams()
        , rcmJamParams()
    {}
};

#endif // TASKPLANNINGDATA_H
