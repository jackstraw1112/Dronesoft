#ifndef RZSIM_ANTI_RADIATION_UAV_TASKPLANNINGDATA_H
#define RZSIM_ANTI_RADIATION_UAV_TASKPLANNINGDATA_H

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>
#include <QUuid>
#include <QMetaType>

// ============================================================================
// 反辐射无人机任务规划系统 — 统一数据结构定义
// 按数据流转的 5 个阶段（任务创建→兵力需求→协同分配→航路规划→参数装订）
// 组织所有跨模块共用的结构体与枚举，作为全工程的数据单一事实来源。
// ============================================================================

// ============================================================================
// 阶段 1：任务创建 (Task Creation)
// 模块：RZSetTaskPlan / SetPointTargetEditDialog / TaskPlanningData
// 产出：TaskBasicInfo + 点目标/区域目标列表
// 下游消费方：兵力需求、协同分配、航路规划、参数装订
// ============================================================================

// 点目标类型枚举：以雷达为主，兼顾非雷达辐射源
enum class TargetType
{
    Unknown = 0,         // 未知/其他
    EarlyWarningRadar,   // 预警雷达（远程搜索）
    SurveillanceRadar,   // 监视/搜索雷达（中近程搜索）
    TrackingRadar,       // 跟踪雷达
    FireControlRadar,    // 火控雷达（制导/照射）
    HeightFindingRadar,  // 测高雷达
    MultiFunctionRadar,  // 多功能相控阵雷达（含搜索+火控）
    GuidanceRadar,       // 制导雷达
    CommStation,         // 通信/指挥台
    DataLinkNode,        // 数传/数据链节点
    Jammer,              // 干扰/电子战设备
    NavAid               // 导航/标信台（TACAN/VOR 等）
};

// 威胁等级
enum class ThreatLevelType
{
    Unknown = 0,         // 未知
    High,                // 高
    Medium,              // 中
    Low                  // 低
};

// 任务类型：对应 SEAD / DEAD / SEARCH
enum class TaskType
{
    Unknown = 0,         // 未知
    SEAD,                // 防空压制（Suppression of Enemy Air Defenses）
    DEAD,                // 防空摧毁（Destruction of Enemy Air Defenses）
    SEARCH               // 游猎扫荡
};

// 任务优先级
enum class PriorityLevel
{
    Unknown = 0,         // 未指定
    P1,                  // 紧急
    P2,                  // 高
    P3                   // 一般
};

// 方案阶段
enum class TaskPlanStage
{
    Unknown = 0,         // 未指定
    Scheme,              // 方案阶段
    Editing,             // 编辑阶段
    Simulate             // 仿真阶段
};

// 任务状态
enum class TaskStatusType
{
    Unknown = 0,         // 未知
    Pending,             // 待执行
    Planning,            // 规划中
    Running              // 执行中
};

// 点目标：对应界面中点目标表格的一行
struct PointTargetInfo
{
    QString         targetId;        // 目标编号
    QString         name;            // 目标名称
    TargetType      type;            // 点目标类型（预警雷达/火控雷达等）
    ThreatLevelType threatLevel;     // 威胁等级
    double          latitude;        // 纬度（度）
    double          longitude;       // 经度（度）
    double          cepMeters;       // 圆概率误差（米）
    QString         band;            // 工作频段
    QString         emissionPattern; // 辐射样式
    QString         priority;        // 优先级
    double          requiredPk;      // 要求杀伤概率

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
    Unknown = 0,         // 未知
    Circle,              // 圆形
    Rectangle,           // 矩形
    Polygon,             // 多边形
    Corridor             // 走廊/带状
};

// 经纬度点，用于多边形/走廊顶点
struct GeoPosition
{
    double latitude  = 0.0;  // 纬度（度）
    double longitude = 0.0;  // 经度（度）
};

// 搜索策略类型（区域目标）
enum class SearchStrategyType
{
    Unknown = 0,         // 未知
    SpiralScan,          // 螺旋扫描
    ParallelLines,       // 平行线扫描
    SectorScan,          // 扇区扫描
    Loiter,              // 盘旋搜索
};

// 区域目标：对应界面中区域目标表格的一行
struct AreaTargetInfo
{
    QString             targetId;        // 目标编号
    QString             name;            // 目标名称
    AreaGeometryType    areaType;        // 区域几何类型
    double              centerLatitude;  // 中心点纬度（度）
    double              centerLongitude; // 中心点经度（度）
    double              radiusKm;        // 搜索半径（千米）
    QList<GeoPosition>  vertices;        // 多边形顶点列表
    QString             expectedEmitters; // 预期辐射源描述
    SearchStrategyType  searchStrategy;  // 搜索策略
    PriorityLevel       priority;        // 优先级

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

// 按阶段与时间判定任务状态
inline TaskStatusType resolveTaskStatus(TaskPlanStage stage,
                                        uint currentTimestampSec,   // 当前时间戳（秒）
                                        uint startTimestampSec,     // 任务开始时间戳（秒）
                                        uint endTimestampSec)       // 任务结束时间戳（秒）
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

// 任务基本信息（步骤 1 核心产出）
struct TaskBasicInfo
{
    QString       taskUid;             // 任务唯一标识（UUID）
    QString       taskName;            // 任务名称
    QString       taskId;              // 任务编号（如 "MSN-20260508-001"）
    TaskType      taskType;            // 任务类型（SEAD/DEAD/SEARCH）
    PriorityLevel priority;            // 优先级
    ThreatLevelType overallThreatLevel; // 总体威胁等级
    TaskStatusType status;             // 任务状态
    uint          startTimestampSec;   // 计划开始时间戳（秒）
    uint          endTimestampSec;     // 计划结束时间戳（秒）
    QString       intent;              // 任务意图/作战目的

    TaskBasicInfo()
        : taskUid(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , taskName()
        , taskId()
        , taskType(TaskType::Unknown)
        , priority(PriorityLevel::Unknown)
        , overallThreatLevel(ThreatLevelType::Unknown)
        , status(TaskStatusType::Unknown)
        , startTimestampSec(0)
        , endTimestampSec(0)
        , intent()
    {}
};

// ============================================================================
// 阶段 2：兵力需求计算 (Force Requirement)
// 模块：ForceRequirementPanel
// 产出：ForceRequirement（每目标计算总架数）+ PtCalcData / ArCalcData
// 下游消费方：协同任务分配 (TaskAllocationPanel)
// ============================================================================

// 点目标计算结果
struct PtCalcData
{
    double damageLevel = 0.9;  // 毁伤等级
    double Pk = 0.95;          // 杀伤概率
    int n = 1;                 // 主要攻击架数
    int backup = 1;            // 备份架数
    int total = 2;             // 总计架数
};

// 区域目标计算结果
struct ArCalcData
{
    int area = 100;             // 区域面积（平方千米）
    double estTargets = 3.0;    // 预估目标数量
    int shotsPerTarget = 2;     // 每目标打击次数
    int backup = 2;             // 备份架数
    int N_search = 4;           // 搜索架数
    int N_strike = 6;           // 打击架数
    int total = 8;              // 总计架数
};

// 兵力需求计算结果（跨模块传递：Step2 → Step3）
struct ForceTargetData
{
    QString id;              // 目标编号（如 "PT-01"）
    QString name;            // 目标名称
    QString type;            // 目标类型（"PT" 点目标 / "AR" 区域目标）
    int aircraftCount = 0;   // 所需总架数
    QString priority = "P1"; // 优先级（"P1" / "P2"）

    ForceTargetData() = default;
    ForceTargetData(const QString &i, const QString &n, const QString &t, int cnt, const QString &pri = "P1")
        : id(i), name(n), type(t), aircraftCount(cnt), priority(pri) {}
};

// ============================================================================
// 阶段 3：协同任务分配 (Task Allocation)
// 模块：TaskAllocationPanel
// 产出：AltPlanData
// 下游消费方：航路规划 (RoutePlanning)
// ============================================================================

// 候选方案数据
struct AltPlanData
{
    QString name;                // 方案名称
    double fVal = 0.0;           // 目标函数值
    int totalAssigned = 0;       // 总分配架数
    QString risk;                // 风险等级
    bool isCurrent = false;      // 当前是否选中
    QList<int> perTargetCounts;  // 每个目标分配的架数列表
};

// ============================================================================
// 阶段 4：航路规划 (Route Planning)
// 模块：RoutePlanning
// 产出：PathPlanning + PathPoint + UavAssignment
// 下游消费方：参数装订 (ParameterLoading)
// ============================================================================

// 无人机任务分配：每架无人机与目标的关联关系
struct UavAssignment
{
    QString uavId;         // 无人机编号（如 "UAV-01"）
    int uavIndex = 0;      // 全局索引（0-based）
    QString targetId;      // 目标编号（如 "PT-01"）
    QString targetName;    // 目标名称
    QString targetType;    // 目标类型（"PT" / "AR"）
};

// 路径点
struct PathPoint
{
    double latitude;     // 纬度（度）
    double longitude;    // 经度（度）
    double altitude;     // 高度（米）
    int pointOrder;      // 点序号
    QString pointType;   // 点类型（起飞/入航/转弯/目标/返航/降落等）
};

// 路径规划结果
struct PathPlanning
{
    QString planName;              // 方案名称
    QString coordinationName;      // 协同规划名称
    QString uavName;               // 无人机名称
    QString relatedTask;           // 关联任务
    int pathPointCount;            // 航路点总数
    QString status;                // 规划状态
    QList<PathPoint> fightPathPoints;  // 突防航路点列表
    QList<PathPoint> searchPathPoints; // 搜索航路点列表
};

// ============================================================================
// 阶段 5：参数装订 (Parameter Loading)
// 模块：ParameterLoading
// 消费方：无人机链路 / 数据链装订
// 使用先前各阶段数据生成最终装订参数
// ============================================================================

// 雷达辐射源参数（对应于 UI 左侧卡片）
struct RadarEmitterParams
{
    double  freqMinGHz = 8.0;       // 频率范围起始（GHz）
    double  freqMaxGHz = 10.0;      // 频率范围终止（GHz）
    double  pulseWidthMinUs = 0.5;  // 脉宽范围起始（微秒）
    double  pulseWidthMaxUs = 2.5;  // 脉宽范围终止（微秒）
    int     prfMinHz = 1000;        // 重频范围起始（Hz）
    int     prfMaxHz = 5000;        // 重频范围终止（Hz）
    int     threatLevelIndex = 0;   // 威胁等级索引（0=高, 1=中, 2=低）
    QString emitterReport;          // 辐射源批报文本
};

// 导引头搜索参数（对应于 UI 右侧上半部分）
struct SeekerSearchParams
{
    int     searchModeIndex = 0;   // 搜索模式（0=直飞归向, 1=等比螺旋, 2=等差螺旋, 3=摆线搜索）
    double  sectorStartDeg = -30.0; // 扇区起始角（度）
    double  sectorEndDeg   = 30.0;  // 扇区终止角（度）
    double  elevMinDeg     = -15.0; // 俯仰下限（度）
    double  elevMaxDeg     = 25.0;  // 俯仰上限（度）
    double  snrThresholdDb = -68.0; // 信噪比门限（dB）
    int     priorityBandIndex = 0;  // 优先频段索引（0=H/I, 1=G, 2=E/F）
    double  searchTimeoutSec = 120.0; // 搜索超时时间（秒）
};

// 炸高参数（对应于 UI 右侧下半部分）
struct BurstHeightParams
{
    double  burstHeightM = 35.0; // 炸高（米）
    int     fuseTypeIndex = 0;   // 引信类型（0=触发引信, 1=近炸引信）
};

// 无人机装订参数汇总：每架无人机绑定一份完整的参数包
struct DroneBindingInfo
{
    QString droneId;              // 无人机编号
    QString modelBand;            // 型号频段
    double  bindingProgress = 0.0; // 装订进度（0.0 ~ 1.0）
    int     seekerStatusIndex = 0; // 导引头装订状态（0=未装订, 1=已装订, 2=校验通过）
    QString targetName;           // 目标名称
    double  presetHeightM = 35.0; // 预设高度（米）
    int     linkQualityPercent = 0; // 链路质量百分比
    int readinessIndex = 0;    // 就绪状态（0=未就绪, 1=就绪）
};

// 无人机资源信息（全局资源池用）
struct UavResource
{
    int uavIndex = 0;          // 无人机全局序号（0~239）
    QString uavName;           // 无人机名称（如 "JWS-01-001"）
    QString typeName;          // 型号名称（如 "JWS-01"）
    bool isAvailable = true;   // 是否可用
};

// ============================================================================
// 主容器：任务规划全量数据（涵盖 5 个阶段）
// ============================================================================

struct TaskPlanningData
{
    // 阶段 1：任务创建
    TaskBasicInfo basicInfo;                  // 任务基本信息
    QList<PointTargetInfo> pointTargets;      // 点目标列表
    QList<AreaTargetInfo> areaTargets;        // 区域目标列表

    // 阶段 2：兵力需求计算
    QList<ForceTargetData> forceRequirements; // 兵力需求列表
    QMap<QString, PtCalcData> forcePtResults; // 点目标计算结果表（key=目标编号）
    QMap<QString, ArCalcData> forceArResults; // 区域目标计算结果表（key=目标编号）

    // 阶段 3：协同任务分配
    QList<AltPlanData> altPlans;              // 备选方案列表

    // 阶段 4：航路规划
    QList<UavAssignment> uavAssignments;      // 无人机分配列表
    QList<PathPlanning> paths;                // 路径规划结果列表

    // 阶段 5：参数装订
    RadarEmitterParams  radarEmitter;         // 雷达辐射源参数
    SeekerSearchParams  seekerParams;         // 导引头搜索参数
    BurstHeightParams   burstHeightParams;    // 炸高参数
    QList<DroneBindingInfo> droneBindings;    // 无人机装订信息列表

    TaskPlanningData()
        : basicInfo()
        , pointTargets()
        , areaTargets()
        , forceRequirements()
        , forcePtResults()
        , forceArResults()
        , altPlans()
        , uavAssignments()
        , paths()
        , radarEmitter()
        , seekerParams()
        , burstHeightParams()
        , droneBindings()
    {}
};



#endif // RZSIM_ANTI_RADIATION_UAV_TASKPLANNINGDATA_H
