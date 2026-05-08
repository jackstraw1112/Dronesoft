#ifndef PATH_PLANNER_H
#define PATH_PLANNER_H

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#endif
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <sstream>
#include <functional>

// ============================================================
// 数据结构定义
// ============================================================

// 地理坐标点（经度 lon、纬度 lat、高度 alt 单位米，speed_mps 当前飞行速度 m/s）
struct GeoPoint {
    double lon;         // 经度，范围 [-180, 180]
    double lat;         // 纬度，范围 [-90, 90]
    double alt;         // 高度（米，相对海平面）
    double speed_mps = 0.0;  // 当前飞行速度（m/s）
};

// 禁飞区（圆形 或 多边形）
// polygon 非空（≥3点）时为多边形禁飞区，优先于圆形参数；
// center/radius_m 对多边形禁飞区可留默认值，内部自动取质心/外接圆。
struct NoFlyZone {
    GeoPoint center;                    // 圆形：圆心；多边形：可留空
    double radius_m  = 0.0;            // 圆形半径（m）；多边形时忽略
    double min_alt   = 0.0;            // 禁飞最低高度（m）
    double max_alt   = -1.0;           // 禁飞最高高度（m，-1 = 无上限）
    std::vector<GeoPoint> polygon;      // ≥3 点时为多边形禁飞区（顶点顺序任意）
};

// 单架无人机的规划结果（waypoints 与 phase_labels 一一对应）
struct UAVPath {
    int uav_id;
    std::vector<GeoPoint> waypoints;
    std::vector<std::string> phase_labels;
    // phase 取值："takeoff" | "cruise"（approach_paths 中）
    //            "search"  | "strike"（attack_paths   中）
};

// 搜索方式
enum class SearchPattern {
    SPIRAL,   // 内收螺旋（默认）
    FIGURE8,  // 8字搜索
};

// 规划输入
struct PlanningInput {
    std::vector<GeoPoint> start_positions;  // 各机起飞点
    std::vector<GeoPoint> target_area;      // 打击区域：1 个点 = 点目标，≥4 个点 = 多边形区域
    std::vector<NoFlyZone> no_fly_zones;
    int uav_count;              // 1～10
    double cruise_alt;          // 平飞高度（米）
    double cruise_alt_max = 0.0;// 平飞最高高度（米）；> cruise_alt 时各机在 [cruise_alt, cruise_alt_max]
                                // 范围内均匀分配，替代固定 20m 等差错层；0 = 不启用
    double search_radius;       // 搜索半径（米）
    double waypoint_spacing_m;  // 散点间距（米）
    double cruise_speed_mps = 60.0;  // 巡航速度（m/s），其余阶段据此推算
    SearchPattern search_pattern = SearchPattern::SPIRAL;  // 搜索方式
};

// ============================================================
// 优化器接口
// ============================================================

// 每架无人机的可优化任务参数（GA/PSO 等算法的决策变量）
struct MissionParams {
    std::vector<double> ingress_angles;  // 各机入场方位偏移（弧度），影响平飞入场角度
    std::vector<double> alt_offsets;     // 各机高度额外偏移（米），叠加到基础错层之上
};

// GA 配置参数
struct GAConfig {
    int    population_size = 50;      // 种群大小
    int    generations     = 100;     // 迭代代数
    double mutation_rate   = 0.15;    // 变异概率（每个基因）
    double crossover_rate  = 0.85;    // 交叉概率
    int    elite_count     = 2;       // 精英保留数量
    double ingress_range   = M_PI / 3.0;  // 入场角搜索范围 ±60°
    double alt_range       = 200.0;       // 高度偏移搜索范围 ±200m
    int    seed            = 42;          // 随机种子（0 = 不固定）
    // 适应度权重
    double w_path_km  = 1.0;   // 总路径长度（km）的权重
    double w_conflict = 50.0;  // 每对冲突的惩罚
    double w_nfz      = 200.0; // 每次禁飞区违规的惩罚
};

// 优化器抽象接口：GA、PSO、SA 等算法统一实现此接口
// fitness_fn 越小越好（最小化问题）
class MissionOptimizer {
public:
    virtual ~MissionOptimizer() = default;
    virtual MissionParams optimize(
        const PlanningInput& input,
        const std::function<double(const MissionParams&)>& fitness_fn) = 0;
    virtual std::string name() const = 0;
};

// 遗传算法优化器
class GeneticOptimizer : public MissionOptimizer {
public:
    explicit GeneticOptimizer(GAConfig cfg = {});
    MissionParams optimize(
        const PlanningInput& input,
        const std::function<double(const MissionParams&)>& fitness_fn) override;
    std::string name() const override { return "GeneticAlgorithm"; }
private:
    GAConfig cfg_;
};

// ============================================================
// 粒子群优化器（PSO）
// ============================================================

struct PSOConfig {
    int    swarm_size  = 40;      // 粒子数
    int    iterations  = 100;     // 迭代次数
    double inertia_w   = 0.72;    // 惯性权重（逐代线性递减至 min_w）
    double min_w       = 0.40;    // 惯性权重下界
    double c1          = 1.49;    // 个体认知系数（向个体最优靠近）
    double c2          = 1.49;    // 社会学习系数（向全局最优靠近）
    double ingress_range = M_PI / 3.0;  // 入场角搜索范围 ±60°
    double alt_range     = 200.0;       // 高度偏移搜索范围 ±200m
    int    seed          = 0;           // 随机种子（0 = 不固定）
};

class PSOOptimizer : public MissionOptimizer {
public:
    explicit PSOOptimizer(PSOConfig cfg = {});
    MissionParams optimize(
        const PlanningInput& input,
        const std::function<double(const MissionParams&)>& fitness_fn) override;
    std::string name() const override { return "PSO"; }
private:
    PSOConfig cfg_;
};

// ============================================================
// 规划输出
// ============================================================

struct PlanningResult {
    std::vector<UAVPath> approach_paths;  // 爬升+平飞，size == uav_count
    std::vector<UAVPath> attack_paths;    // 搜索+打击，size == uav_count
    bool success;
    std::string error_msg;
    std::string optimizer_name;  // 空字符串表示未使用优化器
};

// ============================================================
// 核心接口
// ============================================================

// optimizer = nullptr 时退化为原有确定性规划
PlanningResult planPaths(const PlanningInput& input,
                          MissionOptimizer* optimizer = nullptr);

bool isPathValid(const std::vector<GeoPoint>& path,
                 const std::vector<NoFlyZone>& zones);

bool hasConflict(const UAVPath& a, const UAVPath& b,
                 double separation_m, double time_step_s);

// ============================================================
// 地理坐标辅助函数
// ============================================================

namespace geo {

constexpr double EARTH_RADIUS_M = 6371000.0;
constexpr double DEG_TO_RAD = M_PI / 180.0;
constexpr double RAD_TO_DEG = 180.0 / M_PI;

double haversineDistance(const GeoPoint& a, const GeoPoint& b);
double distance3D(const GeoPoint& a, const GeoPoint& b);
GeoPoint destinationPoint(const GeoPoint& origin, double bearing_rad, double distance_m);
double bearing(const GeoPoint& a, const GeoPoint& b);
GeoPoint lerp(const GeoPoint& a, const GeoPoint& b, double t);

} // namespace geo

#endif // PATH_PLANNER_H
