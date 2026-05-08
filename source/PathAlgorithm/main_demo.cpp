#include "path_planner.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

// ============================================================
// JSON 输出（写入任意 ostream）
// ============================================================

static void printWaypointArray(std::ostream& out, const UAVPath& path, const std::string& indent) {
    for (size_t j = 0; j < path.waypoints.size(); ++j) {
        const auto& wp = path.waypoints[j];
        out << std::fixed << std::setprecision(6);
        out << indent
            << "{\"lon\": "    << wp.lon
            << ", \"lat\": "   << wp.lat
            << ", \"alt\": "   << std::setprecision(1) << wp.alt
            << ", \"speed\": " << std::setprecision(2) << wp.speed_mps
            << ", \"phase\": \"" << path.phase_labels[j] << "\"}";
        if (j + 1 < path.waypoints.size()) out << ",";
        out << "\n";
    }
}

static void outputJSON(std::ostream& out,
                       const PlanningInput& input,
                       const PlanningResult& result) {
    out << std::fixed;
    out << "{\n";
    out << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
    if (!result.optimizer_name.empty())
        out << "  \"optimizer\": \"" << result.optimizer_name << "\",\n";
    if (!result.error_msg.empty())
        out << "  \"error_msg\": \"" << result.error_msg << "\",\n";

    // 打击区域顶点
    out << "  \"target_area\": [\n";
    for (size_t i = 0; i < input.target_area.size(); ++i) {
        const auto& p = input.target_area[i];
        out << std::setprecision(6);
        out << "    {\"lon\": " << p.lon << ", \"lat\": " << p.lat
            << std::setprecision(1) << ", \"alt\": " << p.alt << "}";
        if (i + 1 < input.target_area.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    // 禁飞区
    out << "  \"no_fly_zones\": [\n";
    for (size_t i = 0; i < input.no_fly_zones.size(); ++i) {
        const auto& z = input.no_fly_zones[i];
        out << std::setprecision(6);
        out << "    {";
        if (z.polygon.size() >= 3) {
            out << "\"polygon\": [";
            for (size_t k = 0; k < z.polygon.size(); ++k) {
                out << "{\"lon\": " << z.polygon[k].lon << ", \"lat\": " << z.polygon[k].lat << "}";
                if (k + 1 < z.polygon.size()) out << ", ";
            }
            out << "]";
        } else {
            out << "\"center\": {\"lon\": " << z.center.lon
                << ", \"lat\": " << z.center.lat
                << std::setprecision(1) << ", \"alt\": " << z.center.alt << "}"
                << ", \"radius_m\": " << z.radius_m;
        }
        out << std::setprecision(1)
            << ", \"min_alt\": " << z.min_alt
            << ", \"max_alt\": " << z.max_alt << "}";
        if (i + 1 < input.no_fly_zones.size()) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    // 航迹
    size_t n = result.approach_paths.size();
    out << "  \"uav_paths\": [\n";
    for (size_t i = 0; i < n; ++i) {
        out << "    {\n";
        out << "      \"uav_id\": " << result.approach_paths[i].uav_id << ",\n";
        out << "      \"approach_waypoints\": [\n";
        printWaypointArray(out, result.approach_paths[i], "        ");
        out << "      ],\n";
        out << "      \"attack_waypoints\": [\n";
        printWaypointArray(out, result.attack_paths[i], "        ");
        out << "      ]\n    }";
        if (i + 1 < n) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
}

// ============================================================
// 统计输出（stderr，不污染 JSON）
// ============================================================

static void printStats(const PlanningInput& input, const PlanningResult& result,
                       const char* label) {
    std::cerr << "\n=== " << label << " ===\n";
    std::cerr << "Optimizer : "
              << (result.optimizer_name.empty() ? "none (deterministic)" : result.optimizer_name)
              << "\n";
    std::cerr << "UAV count : " << input.uav_count << "\n";
    std::cerr << "Search    : "
              << (input.search_pattern == SearchPattern::FIGURE8 ? "figure8" : "spiral") << "\n";

    if (input.target_area.size() == 1)
        std::cerr << "Target    : point (" << input.target_area[0].lon
                  << ", " << input.target_area[0].lat << ")\n";
    else
        std::cerr << "Target    : polygon (" << input.target_area.size() << " vertices)\n";

    std::cerr << "NFZ count : " << input.no_fly_zones.size() << "\n";
    std::cerr << "Success   : " << (result.success ? "Yes" : "No") << "\n";
    if (!result.error_msg.empty())
        std::cerr << "Warnings  : " << result.error_msg << "\n";

    size_t grandTotal = 0;
    for (size_t i = 0; i < result.approach_paths.size(); ++i) {
        const auto& ap  = result.approach_paths[i];
        const auto& atk = result.attack_paths[i];
        int tk=0, cr=0, se=0, st=0;
        for (const auto& l : ap.phase_labels)  { if(l=="takeoff")++tk; else if(l=="cruise")++cr; }
        for (const auto& l : atk.phase_labels) { if(l=="search") ++se; else if(l=="strike")++st; }
        size_t total = ap.waypoints.size() + atk.waypoints.size();
        grandTotal  += total;
        const auto& ep = atk.waypoints.empty() ? ap.waypoints.back() : atk.waypoints.back();
        std::cerr << "  UAV " << ap.uav_id << ": " << total << " pts"
                  << "  takeoff=" << tk << " cruise=" << cr
                  << " search=" << se << " strike=" << st
                  << "  endpoint=(" << std::fixed << std::setprecision(5)
                  << ep.lon << ", " << ep.lat << ")\n";
    }
    std::cerr << "Total waypoints: " << grandTotal << "\n";
}

// ============================================================
// main
// ============================================================
static bool writeJSON(const PlanningInput& input,
                      const PlanningResult& result,
                      const std::string& filename) {
    std::ofstream f(filename);
    if (!f) {
        std::cerr << "[ERROR] Cannot open " << filename << " for writing\n";
        return false;
    }
    outputJSON(f, input, result);
    return true;
}

// 多边形禁飞区有效性校验
static bool checkValidity(const PlanningInput& inp, const PlanningResult& res,
                          const char* label) {
    bool allOk = true;
    for (int i = 0; i < (int)res.approach_paths.size(); ++i) {
        UAVPath merged = res.approach_paths[i];
        for (const auto& w : res.attack_paths[i].waypoints)
            merged.waypoints.push_back(w);
        if (!isPathValid(merged.waypoints, inp.no_fly_zones)) {
            std::cerr << "  [FAIL] UAV" << i << " violates NFZ!\n";
            allOk = false;
        }
    }
    std::cerr << "  " << label << " : " << (allOk ? "[PASS]" : "[FAIL]") << "\n";
    return allOk;
}


int main() {
    std::cerr << "UAV Path Planner Demo\n";
    std::cerr << "=====================\n";

    // ============================================================
    // 场景 1：3 架无人机 + 不规则六边形禁飞区 + 点目标
    // 六边形NFZ偏轴放置，迫使无人机从南北两侧分别绕行
    // ============================================================
    {
        PlanningInput input;
        input.uav_count          = 3;
        input.cruise_alt         = 3000.0;
        input.cruise_speed_mps   = 60.0;
        input.search_radius      = 2000.0;
        input.waypoint_spacing_m = 400.0;
        input.search_pattern     = SearchPattern::SPIRAL;
        input.target_area        = {{116.50, 39.85, 30.0}};
        input.start_positions    = {
            {116.20, 40.05, 50.0},
            {116.22, 40.03, 55.0},
            {116.18, 40.04, 48.0}
        };
        NoFlyZone hex; hex.min_alt = 0; hex.max_alt = -1;
        hex.polygon = {
            {116.31, 40.00, 0.0}, {116.36, 40.01, 0.0}, {116.40, 39.98, 0.0},
            {116.39, 39.93, 0.0}, {116.34, 39.91, 0.0}, {116.29, 39.94, 0.0}
        };
        input.no_fly_zones = {hex};

        PlanningResult result = planPaths(input);
        writeJSON(input, result, "output_s1.json");
        printStats(input, result, "Scenario 1: 3 UAV + Hexagon NFZ + Point Target");
        checkValidity(input, result, "Hexagon NFZ validity");
    }

    // ============================================================
    // 场景 2：5 架无人机 + 双不规则多边形禁飞区 + 矩形目标 + GA
    // 五边形NFZ(早段) + 不规则四边形NFZ(中段)，两区之间保留约4km南侧走廊
    // ============================================================
    {
        PlanningInput input;
        input.uav_count          = 5;
        input.cruise_alt         = 3000.0;
        input.cruise_speed_mps   = 60.0;
        input.search_radius      = 2000.0;
        input.waypoint_spacing_m = 400.0;
        input.search_pattern     = SearchPattern::FIGURE8;
        input.target_area = {
            {116.49, 39.84, 30.0}, {116.54, 39.84, 30.0},
            {116.54, 39.80, 30.0}, {116.49, 39.80, 30.0}
        };
        input.start_positions = {
            {116.18, 40.06, 50.0}, {116.20, 40.04, 55.0},
            {116.22, 40.02, 48.0}, {116.17, 40.05, 52.0},
            {116.21, 40.03, 50.0}
        };
        NoFlyZone penta; penta.min_alt = 0; penta.max_alt = -1;
        penta.polygon = {
            {116.28, 40.01, 0.0}, {116.34, 40.02, 0.0}, {116.37, 39.98, 0.0},
            {116.33, 39.94, 0.0}, {116.27, 39.96, 0.0}
        };
        NoFlyZone quad; quad.min_alt = 0; quad.max_alt = -1;
        quad.polygon = {
            {116.40, 39.99, 0.0}, {116.46, 39.97, 0.0},
            {116.45, 39.92, 0.0}, {116.38, 39.93, 0.0}
        };
        input.no_fly_zones = {penta, quad};

        GAConfig gaCfg;
        gaCfg.population_size = 40;
        gaCfg.generations     = 80;
        gaCfg.seed            = 42;
        GeneticOptimizer ga(gaCfg);

        PlanningResult result = planPaths(input, &ga);
        writeJSON(input, result, "output_s2.json");
        printStats(input, result, "Scenario 2: 5 UAV + Pentagon+Quad NFZ + Rect Target + GA");
        checkValidity(input, result, "Dual polygon NFZ validity");
    }

    // ============================================================
    // 场景 3：5 架无人机 + 双不规则NFZ窄走廊 + 五边形目标 + PSO
    // 两个大不规则多边形NFZ在纵向形成约3km宽的南北走廊
    // ============================================================
    {
        PlanningInput input;
        input.uav_count          = 5;
        input.cruise_alt         = 3000.0;
        input.cruise_speed_mps   = 60.0;
        input.search_radius      = 2000.0;
        input.waypoint_spacing_m = 400.0;
        input.search_pattern     = SearchPattern::SPIRAL;
        input.target_area = {
            {116.50, 39.84, 30.0}, {116.54, 39.81, 30.0}, {116.52, 39.78, 30.0},
            {116.48, 39.79, 30.0}, {116.47, 39.82, 30.0}
        };
        input.start_positions = {
            {116.18, 40.06, 50.0}, {116.20, 40.04, 55.0},
            {116.22, 40.02, 48.0}, {116.17, 40.05, 52.0},
            {116.21, 40.03, 50.0}
        };
        NoFlyZone northPenta; northPenta.min_alt = 0; northPenta.max_alt = -1;
        northPenta.polygon = {
            {116.28, 40.03, 0.0}, {116.36, 40.04, 0.0}, {116.42, 40.01, 0.0},
            {116.41, 39.97, 0.0}, {116.29, 39.97, 0.0}
        };
        NoFlyZone southQuad; southQuad.min_alt = 0; southQuad.max_alt = -1;
        southQuad.polygon = {
            {116.30, 39.93, 0.0}, {116.43, 39.92, 0.0},
            {116.44, 39.87, 0.0}, {116.28, 39.88, 0.0}
        };
        input.no_fly_zones = {northPenta, southQuad};

        PSOConfig psoCfg;
        psoCfg.swarm_size = 40;
        psoCfg.iterations = 80;
        psoCfg.seed       = 13;
        PSOOptimizer pso(psoCfg);

        PlanningResult result = planPaths(input, &pso);
        writeJSON(input, result, "output_s3.json");
        printStats(input, result, "Scenario 3: 5 UAV + Narrow Corridor NFZ + Pentagon Target + PSO");
        checkValidity(input, result, "Narrow corridor NFZ validity");
    }

    // ============================================================
    // 场景 4：7 架无人机 + 三不规则多边形NFZ（三角+六边形+四边形）+ 矩形目标 + GA
    // 三个不规则多边形NFZ依次排列在航路上，各具不同形状
    // ============================================================
    {
        PlanningInput input;
        input.uav_count          = 7;
        input.cruise_alt         = 3000.0;
        input.cruise_speed_mps   = 60.0;
        input.search_radius      = 2000.0;
        input.waypoint_spacing_m = 400.0;
        input.search_pattern     = SearchPattern::FIGURE8;
        input.target_area = {
            {116.49, 39.83, 30.0}, {116.54, 39.83, 30.0},
            {116.54, 39.79, 30.0}, {116.49, 39.79, 30.0}
        };
        input.start_positions = {
            {116.16, 40.07, 50.0}, {116.18, 40.05, 52.0},
            {116.20, 40.03, 48.0}, {116.22, 40.01, 55.0},
            {116.15, 40.06, 50.0}, {116.17, 40.04, 53.0},
            {116.21, 40.02, 47.0}
        };
        NoFlyZone tri; tri.min_alt = 0; tri.max_alt = -1;
        tri.polygon = {
            {116.28, 40.01, 0.0}, {116.37, 40.00, 0.0}, {116.35, 39.92, 0.0}
        };
        NoFlyZone hex; hex.min_alt = 0; hex.max_alt = -1;
        hex.polygon = {
            {116.38, 39.99, 0.0}, {116.43, 40.01, 0.0}, {116.45, 39.97, 0.0},
            {116.45, 39.91, 0.0}, {116.40, 39.88, 0.0}, {116.37, 39.91, 0.0}
        };
        NoFlyZone quad; quad.min_alt = 0; quad.max_alt = -1;
        quad.polygon = {
            {116.49, 39.96, 0.0}, {116.54, 39.93, 0.0},
            {116.53, 39.87, 0.0}, {116.47, 39.88, 0.0}
        };
        input.no_fly_zones = {tri, hex, quad};

        GAConfig gaCfg;
        gaCfg.population_size = 40;
        gaCfg.generations     = 80;
        gaCfg.seed            = 7;
        GeneticOptimizer ga(gaCfg);

        PlanningResult result = planPaths(input, &ga);
        writeJSON(input, result, "output_s4.json");
        printStats(input, result, "Scenario 4: 7 UAV + Triangle+Hexagon+Quad NFZ + Rect Target + GA");
        checkValidity(input, result, "Triple polygon NFZ validity");
    }

    // ============================================================
    // 场景 5：10 架无人机 + 三大不规则NFZ极限测试 + 矩形目标
    // 三个大面积不规则多边形NFZ覆盖航路主干，各区间保留约2-3km走廊
    // ============================================================
    {
        PlanningInput input;
        input.uav_count          = 10;
        input.cruise_alt         = 3000.0;
        input.cruise_speed_mps   = 60.0;
        input.search_radius      = 1500.0;
        input.waypoint_spacing_m = 500.0;
        input.search_pattern     = SearchPattern::SPIRAL;
        input.target_area = {
            {116.50, 39.83, 30.0}, {116.55, 39.83, 30.0},
            {116.55, 39.79, 30.0}, {116.50, 39.79, 30.0}
        };
        input.start_positions = {
            {116.14, 40.09, 50.0}, {116.16, 40.07, 52.0},
            {116.18, 40.05, 48.0}, {116.20, 40.03, 55.0},
            {116.22, 40.01, 50.0}, {116.15, 40.08, 53.0},
            {116.17, 40.06, 47.0}, {116.19, 40.04, 51.0},
            {116.21, 40.02, 49.0}, {116.13, 40.10, 54.0}
        };
        NoFlyZone nfz1; nfz1.min_alt = 0; nfz1.max_alt = -1;
        nfz1.polygon = {
            {116.25, 39.99, 0.0}, {116.35, 40.01, 0.0}, {116.37, 39.96, 0.0},
            {116.34, 39.89, 0.0}, {116.23, 39.91, 0.0}
        };
        NoFlyZone nfz2; nfz2.min_alt = 0; nfz2.max_alt = -1;
        nfz2.polygon = {
            {116.39, 40.01, 0.0}, {116.44, 40.03, 0.0}, {116.47, 39.99, 0.0},
            {116.47, 39.92, 0.0}, {116.42, 39.88, 0.0}, {116.38, 39.91, 0.0}
        };
        NoFlyZone nfz3; nfz3.min_alt = 0; nfz3.max_alt = -1;
        nfz3.polygon = {
            {116.49, 39.98, 0.0}, {116.57, 39.96, 0.0},
            {116.56, 39.87, 0.0}, {116.48, 39.89, 0.0}
        };
        input.no_fly_zones = {nfz1, nfz2, nfz3};

        PlanningResult result = planPaths(input);
        writeJSON(input, result, "output_s5.json");
        printStats(input, result, "Scenario 5: 10 UAV + 3 Large NFZ Stress Test + Rect Target");
        checkValidity(input, result, "Three large polygon NFZ validity");
    }

    // ============================================================
    // 场景 6：240 架无人机压力测试 + 3 个大型多边形禁飞区 + 矩形目标
    // 不使用优化器（GA/PSO 在 480 维空间收敛极差，此场景仅测试算法极限）
    // 起飞点：20列×12行网格，间距约1km×1.4km，覆盖约20km×17km区域
    // ============================================================
    {
        PlanningInput input;
        input.uav_count          = 240;
        input.cruise_alt         = 3000.0;
        input.cruise_alt_max     = 5200.0;  // 240机在3000~5200m均匀分配（约9m错层）
        input.cruise_speed_mps   = 60.0;
        input.search_radius      = 1500.0;
        input.waypoint_spacing_m = 500.0;
        input.search_pattern     = SearchPattern::SPIRAL;
        input.target_area = {
            {116.50, 39.83, 30.0}, {116.55, 39.83, 30.0},
            {116.55, 39.79, 30.0}, {116.50, 39.79, 30.0}
        };

        // 20列 × 12行 = 240 个起飞点
        const int COLS = 20, ROWS = 12;
        const double LON0 = 116.08, LAT0 = 40.07;
        const double DLON = 0.013, DLAT = 0.013;
        int id = 0;
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                double alt = 45.0 + (id % 7) * 2.0;
                input.start_positions.push_back({
                    LON0 + c * DLON,
                    LAT0 + r * DLAT,
                    alt
                });
                ++id;
            }
        }

        // 与场景5相同的3个大型禁飞区
        NoFlyZone nfz1; nfz1.min_alt = 0; nfz1.max_alt = -1;
        nfz1.polygon = {
            {116.25, 39.99, 0.0}, {116.35, 40.01, 0.0}, {116.37, 39.96, 0.0},
            {116.34, 39.89, 0.0}, {116.23, 39.91, 0.0}
        };
        NoFlyZone nfz2; nfz2.min_alt = 0; nfz2.max_alt = -1;
        nfz2.polygon = {
            {116.39, 40.01, 0.0}, {116.44, 40.03, 0.0}, {116.47, 39.99, 0.0},
            {116.47, 39.92, 0.0}, {116.42, 39.88, 0.0}, {116.38, 39.91, 0.0}
        };
        NoFlyZone nfz3; nfz3.min_alt = 0; nfz3.max_alt = -1;
        nfz3.polygon = {
            {116.49, 39.98, 0.0}, {116.57, 39.96, 0.0},
            {116.56, 39.87, 0.0}, {116.48, 39.89, 0.0}
        };
        input.no_fly_zones = {nfz1, nfz2, nfz3};

        std::cerr << "\n[Stress Test] Planning 240 UAVs (deterministic, no optimizer)...\n";
        auto t0 = std::chrono::high_resolution_clock::now();

        PlanningResult result = planPaths(input);

        auto t1 = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(t1 - t0).count();
        std::cerr << "[Stress Test] Elapsed: " << std::fixed << std::setprecision(2)
                  << elapsed << "s\n";

        writeJSON(input, result, "output_s6_stress240.json");
        printStats(input, result, "Scenario 6: 240 UAV Stress Test + 3 Large NFZ");
        checkValidity(input, result, "240 UAV NFZ validity");

        // 统计冲突残留
        int conflictPairs = 0;
        for (int i = 0; i < (int)result.approach_paths.size(); ++i) {
            UAVPath mi = result.approach_paths[i];
            for (const auto& w : result.attack_paths[i].waypoints)
                mi.waypoints.push_back(w);
            for (int j = i + 1; j < (int)result.approach_paths.size(); ++j) {
                UAVPath mj = result.approach_paths[j];
                for (const auto& w : result.attack_paths[j].waypoints)
                    mj.waypoints.push_back(w);
                if (hasConflict(mi, mj, 200.0, 1.0)) ++conflictPairs;
            }
        }
        std::cerr << "  Remaining conflict pairs: " << conflictPairs
                  << " / " << (240 * 239 / 2) << " total pairs\n";
    }

    std::cerr << "\n=====================\n";
    std::cerr << "JSON files written:\n";
    std::cerr << "  output_s1.json          (3 UAV + Hexagon NFZ + Point Target)\n";
    std::cerr << "  output_s2.json          (5 UAV + Pentagon+Quad NFZ + Rect Target + GA)\n";
    std::cerr << "  output_s3.json          (5 UAV + Narrow Corridor NFZ + Pentagon Target + PSO)\n";
    std::cerr << "  output_s4.json          (7 UAV + Triangle+Hexagon+Quad NFZ + Rect Target + GA)\n";
    std::cerr << "  output_s5.json          (10 UAV + 3 Large NFZ Stress Test + Rect Target)\n";
    std::cerr << "  output_s6_stress240.json(240 UAV Stress Test + 3 Large NFZ)\n";
    std::cerr << "Open demo_visualization.html and use [导入 C++ JSON] to visualize.\n";
    std::cerr << "=====================\n";

#ifdef _WIN32
    std::cerr << "\nPress Enter to exit...";
    std::cin.get();
#endif

    return 0;
}
