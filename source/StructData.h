#ifndef STRUCTDATA_H
#define STRUCTDATA_H

#include <QString>
#include <QList>
#include <QTime>
#include <QMap>
#include <QMetaType>

/**
 * @brief 任务信息结构体
 * @details 存储任务的基本信息，包括任务名称、类型、目标、时间等
 */
struct TaskInfo {
    QString planName;          // 方案名称
    QString coordinationName;  // 协同规划名称
    QString taskName;          // 任务名称
    QString taskType;          // 任务类型（打击、压制）
    QString targetType;        // 目标类型（点目标、区域目标）
    QString taskTarget;        // 任务目标（目标名称/区域名称）
    QTime startTime;           // 开始时间
    QTime endTime;             // 结束时间
    QString allocatedUAVs;     // 分配无人机

    /**
     * @brief 获取时间范围字符串
     * @return 格式为"HH:mm~HH:mm"的时间范围字符串
     */
    QString getTimeRange() const {
        return startTime.toString("HH:mm") + "~" + endTime.toString("HH:mm");
    }

    /**
     * @brief 重载等于操作符
     * @param other 另一个任务信息结构体
     * @return 是否相等
     */
    bool operator==(const TaskInfo& other) const {
        return planName == other.planName &&
               coordinationName == other.coordinationName &&
               taskName == other.taskName &&
               taskType == other.taskType &&
               targetType == other.targetType &&
               taskTarget == other.taskTarget &&
               startTime == other.startTime &&
               endTime == other.endTime &&
               allocatedUAVs == other.allocatedUAVs;
    }
};

/**
 * @brief 兵力需求计算结构体
 * @details 存储根据任务计算得出的兵力需求信息
 */
struct ForceCalculation {
    QString planName;           // 方案名称
    QString coordinationName;    // 协同规划名称
    QString taskName;          // 任务名称
    QString taskTarget;        // 任务目标
    QString threatLevel;       // 威胁等级（高、中、低）
    QString priority;          // 优先级（P1、P2、P3）
    int calculatedCount;       // 计算数量
    int adjustedCount;         // 调整数量
};



/**
 * @brief 装备型号结构体
 * @details 存储装备型号及其下属的无人机列表
 */
struct UAVInfo {
    QString uavName;           // 无人机名称
    QString status;            // 状态（就绪、待命、故障）

    UAVInfo()
    {
        status="";
    }
};

/**
 * @brief 编组信息结构体
 * @details 存储编组信息及其包含的装备型号列表
 */
struct GroupInfo {
    QString planName;          // 方案名称
    QString coordinationName;  // 协同规划名称
    QString groupName;         // 编组名称
    QString equipmentName;     // 装备名称
    QList<UAVInfo> equipList;  // 该编组下的装备型号列表
};

/**
 * @brief 任务分配结构体
 * @details 存储任务分配给无人机的具体信息
 */
struct TaskAllocation {
    QString planName;          // 方案名称
    QString coordinationName;  // 协同规划名称
    QString taskName;          // 任务名称
    QString targetType;        // 目标类型
    QString taskTarget;        // 任务目标
    QString threatLevel;       // 威胁等级
    QString allocatedUAVs;     // 分配无人机（逗号分隔）
    QString formation;         // 编队
};

/**
 * @brief 路径点结构体
 * @details 存储路径规划中单个点的坐标信息
 */
struct PathPoint {
    double latitude;       // 纬度
    double longitude;      // 经度
    double altitude;       // 高度(m)
    int pointOrder;        // 点序号
    QString pointType;     // 点类型（巡航路径和搜索路径）
};

/**
 * @brief 路径规划结构体
 * @details 存储无人机的路径规划信息
 */
struct PathPlanning {
    QString planName;           // 方案名称
    QString coordinationName;    // 协同规划名称
    QString uavName;           // 无人机名称
    QString relatedTask;       // 关联任务
    int pathPointCount;        // 路径点数
    QString status;            // 状态（已生成、待生成）
    QList<PathPoint> fightPathPoints; // 飞行路径点列表
    QList<PathPoint> searchPathPoints; // 搜索路径点列表
};

/**
 * @brief 雷达目标参数结构体
 * @details 存储雷达目标的详细参数信息
 */
struct RadarTargetParam {
    QString planName;          // 方案名称
    QString coordinationName;  // 协同规划名称
    QString targetId;          // 目标编号
    double frequencyMin;       // 频率下限(MHz)
    double frequencyMax;       // 频率上限(MHz)
    double pulseWidthMin;      // 脉宽下限(μs)
    double pulseWidthMax;      // 脉宽上限(μs)
    double repetitionPeriodMin;  // 重复周期下限(ms)
    double repetitionPeriodMax;  // 重复周期上限(ms)
    QString workingMode;       // 工作模式
};

/**
 * @brief 电台目标参数结构体
 * @details 存储无线电目标的详细参数信息
 */
struct RadioTargetParam {
    QString planName;           // 方案名称
    QString coordinationName;    // 协同规划名称
    QString targetId;          // 目标编号
    QString frequencyRange;    // 频率范围(MHz)
    QString modulationMode;    // 调制方式
    QString signalBandwidth;   // 信号带宽(kHz)
    QString transmitPower;     // 发射功率(kW)
};

/**
 * @brief 通信对抗参数结构体
 * @details 存储通信干扰的详细参数信息
 */
struct CommJammingParam {
    QString planName;           // 方案名称
    QString coordinationName;    // 协同规划名称
    QString targetId;          // 目标编号
    QString jammingFrequency;  // 干扰频率(MHz)
    QString jammingMode;       // 干扰样式
    QString jammingPower;      // 干扰功率(kW)
    QString coverageRange;     // 覆盖范围(km)
};

/**
 * @brief 雷达对抗参数结构体
 * @details 存储雷达干扰的详细参数信息
 */
struct RcmJammingParam {
    QString planName;           // 方案名称
    QString coordinationName;    // 协同规划名称
    QString targetId;          // 目标编号
    QString jammingFrequency;  // 干扰频率(GHz)
    QString jammingMode;       // 干扰样式
    QString jammingPower;      // 干扰功率(kW)
    QString coverageRange;     // 覆盖范围(km)
};


// 雷达目标数据结构：用于兵力需求面板的目标选择
struct RadarTarget {
    QString id;      // 编号，如 PT-01
    QString name;    // 名称，如 东郊制导雷达
    QString type;    // 类型：PT(炮台雷达) 或 AR(防空区)
    RadarTarget(const QString& i = QString(), const QString& n = QString(), const QString& t = "PT")
        : id(i), name(n), type(t) {}
};

Q_DECLARE_METATYPE(RadarTarget);

// 点目标计算结果数据结构：存储每个PT目标的计算参数和结果
struct PtCalcData {
    double damageLevel = 0.9;  // 毁伤要求P̄
    double Pk = 0.95;           // 单弹毁伤概率
    int n = 1;                  // 计算所需弹数
    int backup = 1;             // 备份架数
    int total = 2;              // 总架数 = n + backup
};

// 区域目标计算结果数据结构：存储每个AR目标的计算参数和结果
struct ArCalcData {
    int area = 100;             // 区域面积 KM²
    double estTargets = 3.0;    // 估计目标数
    int shotsPerTarget = 2;     // 每目标弹数
    int backup = 2;             // 备份架数
    int N_search = 4;           // 搜索所需架数
    int N_strike = 6;           // 打击所需架数
    int total = 8;              // 总架数 = MAX(N_search,N_strike) + backup
};


#endif // STRUCTDATA_H
