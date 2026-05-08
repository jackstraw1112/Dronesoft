#include "path_planner.h"
#include <iostream>
#include <set>
#include <limits>
#include <cassert>
#include <random>
#include <array>
#include <functional>

// ============================================================
// 地理坐标辅助函数实现
// ============================================================

namespace geo {

// Haversine 公式：计算两个经纬度点之间的地表距离（米）
double haversineDistance(const GeoPoint& a, const GeoPoint& b) {
    double lat1 = a.lat * DEG_TO_RAD;
    double lat2 = b.lat * DEG_TO_RAD;
    double dlat = (b.lat - a.lat) * DEG_TO_RAD;
    double dlon = (b.lon - a.lon) * DEG_TO_RAD;

    double sinDlat = std::sin(dlat / 2.0);
    double sinDlon = std::sin(dlon / 2.0);
    double ha = sinDlat * sinDlat + std::cos(lat1) * std::cos(lat2) * sinDlon * sinDlon;
    double c = 2.0 * std::atan2(std::sqrt(ha), std::sqrt(1.0 - ha));
    return EARTH_RADIUS_M * c;
}

// 三维距离
double distance3D(const GeoPoint& a, const GeoPoint& b) {
    double hDist = haversineDistance(a, b);
    double dalt = b.alt - a.alt;
    return std::sqrt(hDist * hDist + dalt * dalt);
}

// 根据起点、方位角和距离计算目标经纬度（球面三角学）
GeoPoint destinationPoint(const GeoPoint& origin, double bearing_rad, double distance_m) {
    double lat1 = origin.lat * DEG_TO_RAD;
    double lon1 = origin.lon * DEG_TO_RAD;
    double d = distance_m / EARTH_RADIUS_M;

    double lat2 = std::asin(std::sin(lat1) * std::cos(d) +
                            std::cos(lat1) * std::sin(d) * std::cos(bearing_rad));
    double lon2 = lon1 + std::atan2(std::sin(bearing_rad) * std::sin(d) * std::cos(lat1),
                                     std::cos(d) - std::sin(lat1) * std::sin(lat2));

    GeoPoint result;
    result.lat = lat2 * RAD_TO_DEG;
    result.lon = lon2 * RAD_TO_DEG;
    result.alt = origin.alt;
    return result;
}

// 方位角（弧度，0=北，顺时针）
double bearing(const GeoPoint& a, const GeoPoint& b) {
    double lat1 = a.lat * DEG_TO_RAD;
    double lat2 = b.lat * DEG_TO_RAD;
    double dlon = (b.lon - a.lon) * DEG_TO_RAD;

    double y = std::sin(dlon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2) -
               std::sin(lat1) * std::cos(lat2) * std::cos(dlon);
    return std::atan2(y, x);
}

// 线性插值（含速度）
GeoPoint lerp(const GeoPoint& a, const GeoPoint& b, double t) {
    GeoPoint result;
    result.lon = a.lon + (b.lon - a.lon) * t;
    result.lat = a.lat + (b.lat - a.lat) * t;
    result.alt = a.alt + (b.alt - a.alt) * t;
    result.speed_mps = a.speed_mps + (b.speed_mps - a.speed_mps) * t;
    return result;
}

} // namespace geo

// ============================================================
// ============================================================
// 多边形禁飞区几何辅助
// ============================================================

// 2D 线段相交（含端点）
static bool seg2DIntersect(double ax, double ay, double bx, double by,
                            double cx, double cy, double dx, double dy) {
    double d1x=bx-ax, d1y=by-ay, d2x=dx-cx, d2y=dy-cy;
    double denom = d1x*d2y - d1y*d2x;
    if (std::abs(denom) < 1e-12) return false;
    double t = ((cx-ax)*d2y - (cy-ay)*d2x) / denom;
    double u = ((cx-ax)*d1y - (cy-ay)*d1x) / denom;
    return t >= 0 && t <= 1 && u >= 0 && u <= 1;
}

// 射线法判点是否在多边形内，bufferFactor>1 时等比放大多边形（以原点为中心）
static bool ptInPolyXY(double px, double py,
                        const std::vector<std::pair<double,double>>& poly,
                        double bf = 1.0) {
    bool inside = false;
    size_t n = poly.size();
    for (size_t i = 0, j = n-1; i < n; j = i++) {
        double ix = poly[i].first*bf, iy = poly[i].second*bf;
        double jx = poly[j].first*bf, jy = poly[j].second*bf;
        if (((iy > py) != (jy > py)) && (px < (jx-ix)*(py-iy)/(jy-iy)+ix))
            inside = !inside;
    }
    return inside;
}

// 禁飞区外接圆：圆形→{center, radius}；多边形→{质心, 最远顶点距}
static std::pair<GeoPoint, double> nfzCircumscribed(const NoFlyZone& zone) {
    if (zone.polygon.size() >= 3) {
        GeoPoint c{0, 0, 0};
        for (const auto& v : zone.polygon) { c.lon += v.lon; c.lat += v.lat; }
        double n = (double)zone.polygon.size();
        c.lon /= n; c.lat /= n;
        double r = 0;
        for (const auto& v : zone.polygon)
            r = std::max(r, geo::haversineDistance(c, v));
        return {c, r};
    }
    return {zone.center, zone.radius_m};
}

// 以质心为原点，将多边形顶点投影到米制 XY
static std::vector<std::pair<double,double>> polyToLocalXY(
        const std::vector<GeoPoint>& poly, const GeoPoint& centroid) {
    double lat0 = centroid.lat * geo::DEG_TO_RAD;
    double cos_lat0 = std::cos(lat0);
    std::vector<std::pair<double,double>> out(poly.size());
    for (size_t i = 0; i < poly.size(); ++i) {
        out[i].first  = (poly[i].lon - centroid.lon) * geo::DEG_TO_RAD * cos_lat0 * geo::EARTH_RADIUS_M;
        out[i].second = (poly[i].lat - centroid.lat) * geo::DEG_TO_RAD * geo::EARTH_RADIUS_M;
    }
    return out;
}

// ============================================================
// 禁飞区检测
// ============================================================

static bool isPointInNoFlyZone(const GeoPoint& point, const NoFlyZone& zone, double buffer = 1.05) {
    if (point.alt < zone.min_alt) return false;
    if (zone.max_alt > 0 && point.alt > zone.max_alt) return false;

    if (zone.polygon.size() >= 3) {
        auto [c, r_] = nfzCircumscribed(zone);
        // 快速拒绝
        if (geo::haversineDistance(point, c) > r_ * buffer * 1.5) return false;
        auto polyXY = polyToLocalXY(zone.polygon, c);
        double lat0 = c.lat * geo::DEG_TO_RAD, cos_lat0 = std::cos(lat0);
        double px = (point.lon - c.lon) * geo::DEG_TO_RAD * cos_lat0 * geo::EARTH_RADIUS_M;
        double py = (point.lat - c.lat) * geo::DEG_TO_RAD * geo::EARTH_RADIUS_M;
        return ptInPolyXY(px, py, polyXY, buffer);
    }
    double dist = geo::haversineDistance(point, zone.center);
    return dist < zone.radius_m * buffer;
}

bool isPathValid(const std::vector<GeoPoint>& path, const std::vector<NoFlyZone>& zones) {
    for (const auto& point : path)
        for (const auto& zone : zones)
            if (isPointInNoFlyZone(point, zone)) return false;
    return true;
}

// 线段与禁飞区相交：圆形用点到段最短距离，多边形用射线法+边相交
static bool lineIntersectsNoFlyZone(const GeoPoint& a, const GeoPoint& b,
                                     const NoFlyZone& zone, double buffer = 1.05) {
    double midAlt = 0.5 * (a.alt + b.alt);
    if (midAlt < zone.min_alt) return false;
    if (zone.max_alt > 0 && midAlt > zone.max_alt) return false;

    if (zone.polygon.size() >= 3) {
        auto [c, r_] = nfzCircumscribed(zone);
        double lat0 = c.lat * geo::DEG_TO_RAD, cos_lat0 = std::cos(lat0);
        auto proj = [&](const GeoPoint& p) {
            return std::pair<double,double>{
                (p.lon - c.lon) * geo::DEG_TO_RAD * cos_lat0 * geo::EARTH_RADIUS_M,
                (p.lat - c.lat) * geo::DEG_TO_RAD * geo::EARTH_RADIUS_M
            };
        };
        auto polyXY = polyToLocalXY(zone.polygon, c);
        auto [ax, ay] = proj(a);
        auto [bx, by] = proj(b);
        // 端点在扩大多边形内
        if (ptInPolyXY(ax, ay, polyXY, buffer)) return true;
        if (ptInPolyXY(bx, by, polyXY, buffer)) return true;
        // 线段与各多边形边相交
        size_t n = polyXY.size();
        for (size_t i = 0, j = n-1; i < n; j = i++) {
            if (seg2DIntersect(ax, ay, bx, by,
                               polyXY[i].first*buffer, polyXY[i].second*buffer,
                               polyXY[j].first*buffer, polyXY[j].second*buffer))
                return true;
        }
        return false;
    }

    // 圆形：点到线段最短距离
    double lat0 = zone.center.lat * geo::DEG_TO_RAD;
    double cos_lat0 = std::cos(lat0);
    auto proj = [&](const GeoPoint& p) {
        double x = (p.lon - zone.center.lon) * geo::DEG_TO_RAD * cos_lat0 * geo::EARTH_RADIUS_M;
        double y = (p.lat - zone.center.lat) * geo::DEG_TO_RAD * geo::EARTH_RADIUS_M;
        return std::pair<double,double>{x, y};
    };
    auto [ax, ay] = proj(a);
    auto [bx, by] = proj(b);
    double dx = bx-ax, dy = by-ay, len2 = dx*dx + dy*dy;
    double cx, cy;
    if (len2 < 1e-9) { cx = ax; cy = ay; }
    else {
        double t = -(ax*dx + ay*dy) / len2;
        if (t < 0) t = 0; else if (t > 1) t = 1;
        cx = ax + t*dx; cy = ay + t*dy;
    }
    return std::sqrt(cx*cx + cy*cy) < zone.radius_m * buffer;
}

// ============================================================
// A* 栅格避障（本地 equirectangular 投影到米制平面）
// 用于 generateCruise 绕避禁飞区。失败时由几何切点法（generateBypassPoints）兜底。
// 栅格边长约 spacing_m×2，8 邻接，防止对角穿墙。
// 返回折线上的粗略节点（含起终点），已做视线平滑。
// ============================================================
static std::vector<GeoPoint> astarCruiseRoute(
    const GeoPoint& from, const GeoPoint& to, double cruise_alt,
    const std::vector<NoFlyZone>& zones, double cell_size_m)
{
    // 所有影响 cruise_alt 的禁飞区
    std::vector<const NoFlyZone*> activeZones;
    for (const auto& z : zones) {
        if (cruise_alt < z.min_alt) continue;
        if (z.max_alt > 0 && cruise_alt > z.max_alt) continue;
        activeZones.push_back(&z);
    }

    // 直线无阻则直接返回，省去 A* 计算
    bool anyBlock = false;
    for (const auto* z : activeZones) {
        if (lineIntersectsNoFlyZone(from, to, *z, 1.05)) { anyBlock = true; break; }
    }
    if (!anyBlock) return {from, to};

    // 本地 equirectangular 投影（以 from 为参考原点，十公里级范围误差可忽略）
    double lat0_rad = from.lat * geo::DEG_TO_RAD;
    double lon0_rad = from.lon * geo::DEG_TO_RAD;
    double cos_lat0 = std::cos(lat0_rad);
    auto toXY = [&](const GeoPoint& p, double& x, double& y) {
        x = (p.lon * geo::DEG_TO_RAD - lon0_rad) * cos_lat0 * geo::EARTH_RADIUS_M;
        y = (p.lat * geo::DEG_TO_RAD - lat0_rad) * geo::EARTH_RADIUS_M;
    };
    auto fromXY = [&](double x, double y, double alt) {
        GeoPoint r;
        r.lat = (lat0_rad + y / geo::EARTH_RADIUS_M) * geo::RAD_TO_DEG;
        r.lon = (lon0_rad + x / (cos_lat0 * geo::EARTH_RADIUS_M)) * geo::RAD_TO_DEG;
        r.alt = alt;
        return r;
    };

    double fx, fy, tx, ty;
    toXY(from, fx, fy);
    toXY(to, tx, ty);

    // 包围盒：包含起终点与相关禁飞区，外扩 500m
    double minX = std::min(fx, tx), maxX = std::max(fx, tx);
    double minY = std::min(fy, ty), maxY = std::max(fy, ty);
    for (const auto* z : activeZones) {
        auto [zc, zr] = nfzCircumscribed(*z);
        double zx, zy; toXY(zc, zx, zy);
        double r = zr * 1.2;
        minX = std::min(minX, zx - r); maxX = std::max(maxX, zx + r);
        minY = std::min(minY, zy - r); maxY = std::max(maxY, zy + r);
    }
    double pad = std::max(500.0, cell_size_m * 2);
    minX -= pad; maxX += pad; minY -= pad; maxY += pad;

    int W = std::max(4, (int)std::ceil((maxX - minX) / cell_size_m));
    int H = std::max(4, (int)std::ceil((maxY - minY) / cell_size_m));
    // 栅格总数上限，过大则放大 cell_size_m
    const int MAX_CELLS = 40000;
    if ((long long)W * H > MAX_CELLS) {
        double scale = std::sqrt((double)W * H / MAX_CELLS);
        cell_size_m *= scale;
        W = std::max(4, (int)std::ceil((maxX - minX) / cell_size_m));
        H = std::max(4, (int)std::ceil((maxY - minY) / cell_size_m));
    }

    auto idxOf = [W](int cx, int cy) { return cy * W + cx; };
    auto cellCenter = [&](int cx, int cy, double& x, double& y) {
        x = minX + (cx + 0.5) * cell_size_m;
        y = minY + (cy + 0.5) * cell_size_m;
    };

    // 预标记障碍格：外接圆粗拒绝 + isPointInNoFlyZone 精确判断
    // 旧版用外接圆近似会将三角形/不规则多边形的外接圆整体阻塞，导致：
    //   1) 两个紧邻多边形间的实际通道被误堵
    //   2) 三角形禁飞区周围留出的裕量远大于实际需要
    // 新版先用外接圆做快速跳过，再按实际形状精确检测。
    std::vector<char> blocked(W * H, 0);
    std::vector<std::pair<double,double>> zoneXY;
    std::vector<double> zoneEffR;
    zoneXY.reserve(activeZones.size());
    zoneEffR.reserve(activeZones.size());
    for (const auto* z : activeZones) {
        auto [zc, zr] = nfzCircumscribed(*z);
        double zx, zy; toXY(zc, zx, zy);
        zoneXY.push_back({zx, zy});
        zoneEffR.push_back(zr);
    }
    double halfDiag = cell_size_m * 0.7071067811865476;
    for (int cy = 0; cy < H; ++cy) {
        for (int cx = 0; cx < W; ++cx) {
            double x, y; cellCenter(cx, cy, x, y);
            // 粗拒绝：不在任何禁飞区外接圆 1.2× 范围内则跳过
            bool nearAny = false;
            for (size_t zi = 0; zi < activeZones.size(); ++zi) {
                double dx = x - zoneXY[zi].first;
                double dy = y - zoneXY[zi].second;
                double qr = zoneEffR[zi] * 1.2 + halfDiag * 2;
                if (dx*dx + dy*dy < qr*qr) { nearAny = true; break; }
            }
            if (!nearAny) continue;
            // 精确检测：按实际多边形/圆形形状判断，缓冲 5%
            GeoPoint cellPt = fromXY(x, y, cruise_alt);
            for (const auto* z : activeZones) {
                if (isPointInNoFlyZone(cellPt, *z, 1.05)) {
                    blocked[idxOf(cx, cy)] = 1;
                    break;
                }
            }
        }
    }

    auto worldToCell = [&](double x, double y, int& cx, int& cy) {
        cx = (int)((x - minX) / cell_size_m);
        cy = (int)((y - minY) / cell_size_m);
        if (cx < 0) cx = 0; if (cx >= W) cx = W - 1;
        if (cy < 0) cy = 0; if (cy >= H) cy = H - 1;
    };
    int sx, sy, gx, gy;
    worldToCell(fx, fy, sx, sy);
    worldToCell(tx, ty, gx, gy);
    // 起终点强制可通行（可能落在禁飞区外接缓冲里）
    blocked[idxOf(sx, sy)] = 0;
    blocked[idxOf(gx, gy)] = 0;

    std::vector<double> gScore(W * H, std::numeric_limits<double>::infinity());
    std::vector<int> cameFrom(W * H, -1);
    gScore[idxOf(sx, sy)] = 0.0;
    auto heur = [&](int cx, int cy) {
        double dx = (double)(gx - cx) * cell_size_m;
        double dy = (double)(gy - cy) * cell_size_m;
        return std::sqrt(dx*dx + dy*dy);
    };

    using PQE = std::pair<double, int>;
    std::priority_queue<PQE, std::vector<PQE>, std::greater<PQE>> open;
    open.push({heur(sx, sy), idxOf(sx, sy)});
    const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
    const double costs[8] = {1,1,1,1,1.41421356,1.41421356,1.41421356,1.41421356};
    int goalIdx = idxOf(gx, gy);
    bool found = false;

    while (!open.empty()) {
        auto top = open.top(); open.pop();
        int cur = top.second;
        if (cur == goalIdx) { found = true; break; }
        int cy = cur / W, cx = cur % W;
        double curG = gScore[cur];
        if (top.first > curG + heur(cx, cy) + 1e-6) continue; // stale
        for (int k = 0; k < 8; ++k) {
            int nx = cx + dirs[k][0], ny = cy + dirs[k][1];
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            int ni = idxOf(nx, ny);
            if (blocked[ni]) continue;
            // 对角时禁止从两边都是障碍的夹缝穿过
            if (dirs[k][0] != 0 && dirs[k][1] != 0) {
                if (blocked[idxOf(cx + dirs[k][0], cy)] && blocked[idxOf(cx, cy + dirs[k][1])])
                    continue;
            }
            double tentative = curG + costs[k] * cell_size_m;
            if (tentative < gScore[ni]) {
                gScore[ni] = tentative;
                cameFrom[ni] = cur;
                open.push({tentative + heur(nx, ny), ni});
            }
        }
    }

    if (!found) return {};

    std::vector<int> path;
    for (int cur = goalIdx; cur != -1; cur = cameFrom[cur]) path.push_back(cur);
    std::reverse(path.begin(), path.end());

    std::vector<GeoPoint> raw;
    raw.push_back(from);
    for (size_t i = 1; i + 1 < path.size(); ++i) {
        int cx = path[i] % W, cy = path[i] / W;
        double x, y; cellCenter(cx, cy, x, y);
        raw.push_back(fromXY(x, y, cruise_alt));
    }
    raw.push_back(to);

    // 视线平滑：从 anchor 开始往前找最远能直达的点
    std::vector<GeoPoint> smooth;
    smooth.push_back(raw[0]);
    size_t anchor = 0;
    while (anchor < raw.size() - 1) {
        size_t farthest = anchor + 1;
        for (size_t k = anchor + 2; k < raw.size(); ++k) {
            bool clear = true;
            for (const auto* z : activeZones) {
                if (lineIntersectsNoFlyZone(raw[anchor], raw[k], *z, 1.05)) { clear = false; break; }
            }
            if (clear) farthest = k; else break;
        }
        smooth.push_back(raw[farthest]);
        anchor = farthest;
    }
    return smooth;
}

// ============================================================
// 冲突检测
// ============================================================

// 公共接口：比对两条完整路径（单一 waypoints vector）是否时序冲突
bool hasConflict(const UAVPath& a, const UAVPath& b,
                 double separation_m, double time_step_s) {
    if (a.waypoints.empty() || b.waypoints.empty()) return false;

    // 终端汇聚（strike 末段）由设计决定会在同一目标附近相遇，不算冲突
    constexpr size_t TERMINAL_SKIP = 5;
    size_t lenA = a.waypoints.size() > TERMINAL_SKIP ? a.waypoints.size() - TERMINAL_SKIP : 1;
    size_t lenB = b.waypoints.size() > TERMINAL_SKIP ? b.waypoints.size() - TERMINAL_SKIP : 1;

    size_t n = std::min(lenA, lenB);
    for (size_t i = 0; i < n; ++i) {
        if (geo::distance3D(a.waypoints[i], b.waypoints[i]) < separation_m)
            return true;
    }
    return false;
}

// 内部辅助：将 approach + attack 合并为一条完整路径，供冲突检测使用
static UAVPath mergePaths(const UAVPath& approach, const UAVPath& attack) {
    UAVPath merged;
    merged.uav_id = approach.uav_id;
    merged.waypoints = approach.waypoints;
    merged.waypoints.insert(merged.waypoints.end(),
                             attack.waypoints.begin(), attack.waypoints.end());
    merged.phase_labels = approach.phase_labels;
    merged.phase_labels.insert(merged.phase_labels.end(),
                                attack.phase_labels.begin(), attack.phase_labels.end());
    return merged;
}

// ============================================================
// 阶段 1：起飞（takeoff）
// 带弧度爬升：从起飞点向目标方向做抛物线爬升至巡航高度
// 水平位移 = 高度差 × 3.0（≈ 18° 爬升角，匹配典型固定翼察打无人机
// 如 MQ-1 ~12°、MQ-9 ~10°、Switchblade ~25°，18° 居于中位）
// 用 sin 曲线做平滑 S 型加速爬升（先慢后快再慢）
//
// 禁飞区规避：以 15° 步长在 ±165° 范围内搜索不穿越 NFZ 的最近替代方向。
// 沿替代方向完成爬升后，巡航段 A* 再导航到目标，整体路径仍正确。
// 注：从 c=1 开始检测（跳过起点），防止起点本身在 NFZ 内时全向失败。
// ============================================================
static void generateTakeoff(UAVPath& path, const GeoPoint& start,
                            const GeoPoint& target, double cruise_alt,
                            double spacing_m, double cruise_speed_mps,
                            const std::vector<NoFlyZone>& zones) {
    double alt_diff = cruise_alt - start.alt;
    if (alt_diff <= 0) {
        GeoPoint wp = start;
        wp.speed_mps = cruise_speed_mps;
        path.waypoints.push_back(wp);
        path.phase_labels.push_back("takeoff");
        return;
    }

    double horizontal_dist = alt_diff * 3.0;
    double bear = geo::bearing(start, target);

    // 沿给定方位角检测起飞弧（12 个采样点）是否穿越禁飞区
    auto isTakeoffClear = [&](double testBear) -> bool {
        const int C = 12;
        for (int c = 1; c <= C; ++c) {
            double t = (double)c / C;
            double alt_t = 0.5 - 0.5 * std::cos(t * M_PI);
            GeoPoint wp = geo::destinationPoint(start, testBear, horizontal_dist * t);
            wp.alt = start.alt + alt_diff * alt_t;
            for (const auto& z : zones) {
                if (isPointInNoFlyZone(wp, z, 1.1)) return false;
            }
        }
        return true;
    };

    if (!isTakeoffClear(bear)) {
        bool found = false;
        // 以 15° 步长双侧搜索，最多偏转 ±165°
        for (int step = 1; step <= 11 && !found; ++step) {
            double off = step * (M_PI / 12.0);
            if      (isTakeoffClear(bear + off)) { bear += off; found = true; }
            else if (isTakeoffClear(bear - off)) { bear -= off; found = true; }
        }
        // 全向搜索失败则保留原始方向（兜底，起点本身在 NFZ 内的极端情况）
    }

    int steps = std::max(2, std::min(3, (int)(horizontal_dist / (spacing_m * 2.0))));
    for (int i = 0; i <= steps; ++i) {
        double t = (double)i / steps;
        double alt_t = 0.5 - 0.5 * std::cos(t * M_PI); // S 曲线 0→1
        GeoPoint wp = geo::destinationPoint(start, bear, horizontal_dist * t);
        wp.alt = start.alt + alt_diff * alt_t;
        // 速度随高度同步 S 曲线：从 30% 巡航爬升到 100% 巡航
        wp.speed_mps = cruise_speed_mps * (0.3 + 0.7 * alt_t);
        path.waypoints.push_back(wp);
        path.phase_labels.push_back("takeoff");
    }
}

// ============================================================
// 阶段 2：平飞（cruise）
// 恒定巡航高度，A* 栅格避障为主、几何切点绕行为兜底
// ============================================================

static std::vector<GeoPoint> generateBypassPoints(const GeoPoint& from, const GeoPoint& to,
                                                    const NoFlyZone& zone, double cruise_alt) {
    auto [C, R] = nfzCircumscribed(zone);
    const double bd = R * 1.1;

    double d1 = geo::haversineDistance(from, C);
    double d2 = geo::haversineDistance(to,   C);

    // 端点在绕行圆内：退化到弧中点
    auto fallbackMid = [&]() -> std::vector<GeoPoint> {
        double bf = geo::bearing(C, from), bt = geo::bearing(C, to);
        double da = bt - bf;
        while (da >  M_PI) da -= 2*M_PI;
        while (da < -M_PI) da += 2*M_PI;
        GeoPoint wp = geo::destinationPoint(C, bf + da*0.5, bd);
        wp.alt = cruise_alt;
        return {wp};
    };

    if (d1 < bd || d2 < bd) return fallbackMid();

    // 切线半角
    double a1 = std::asin(std::min(1.0, bd / d1));
    double a2 = std::asin(std::min(1.0, bd / d2));

    // 各端点到圆心的方位
    double b1 = geo::bearing(from, C);
    double b2 = geo::bearing(to,   C);

    // 圆心在 from→to 的哪一侧（经纬度叉积，近似有效）
    double cross = (to.lon - from.lon) * (C.lat - from.lat)
                 - (to.lat - from.lat) * (C.lon - from.lon);
    double sign = cross > 0.0 ? -1.0 : +1.0;

    double dir1 = b1 + sign * a1;
    double dir2 = b2 - sign * a2;

    // 等角投影到以 from 为原点的局部米制 XY
    double lat0R   = from.lat * geo::DEG_TO_RAD;
    double cosLat  = std::cos(lat0R);
    double lon2m   = cosLat * geo::DEG_TO_RAD * geo::EARTH_RADIUS_M;
    double lat2m   = geo::DEG_TO_RAD * geo::EARTH_RADIUS_M;

    double tx = (to.lon - from.lon) * lon2m;
    double ty = (to.lat - from.lat) * lat2m;

    double dx1 = std::sin(dir1), dy1 = std::cos(dir1);
    double dx2 = std::sin(dir2), dy2 = std::cos(dir2);

    double det = dy1*dx2 - dx1*dy2;
    if (std::abs(det) < 1e-9) return fallbackMid();

    double t = (ty*dx2 - tx*dy2) / det;
    double s = (dx1*ty - dy1*tx) / det;

    if (t <= 0.0 || s <= 0.0 || t > d1 * 3.0) return fallbackMid();

    // 唯一绕行点：两切线交点，from→P 和 P→to 均不穿外接圆
    GeoPoint wp;
    wp.lon = from.lon + (t * dx1) / lon2m;
    wp.lat = from.lat + (t * dy1) / lat2m;
    wp.alt = cruise_alt;
    return {wp};
}

// 在一对锚点之间求可行路径：优先 A*；失败时退回迭代切点绕行
static std::vector<GeoPoint> solveSegment(const GeoPoint& a, const GeoPoint& b,
                                           double cruise_alt, double spacing_m,
                                           const std::vector<NoFlyZone>& zones) {
    auto astar = astarCruiseRoute(a, b, cruise_alt, zones,
                                  std::max(spacing_m * 2.0, 300.0));
    if (!astar.empty()) return astar;

    // 兜底：旧的迭代切点绕行
    std::vector<GeoPoint> seg = {a, b};
    bool redo = true;
    int maxIter = 5;
    while (redo && maxIter-- > 0) {
        redo = false;
        std::vector<GeoPoint> ns = {seg[0]};
        for (size_t i = 0; i + 1 < seg.size(); ++i) {
            const NoFlyZone* blk = nullptr;
            for (const auto& z : zones) {
                if (cruise_alt < z.min_alt) continue;
                if (z.max_alt > 0 && cruise_alt > z.max_alt) continue;
                if (lineIntersectsNoFlyZone(seg[i], seg[i+1], z)) { blk = &z; break; }
            }
            if (blk) {
                auto bypass = generateBypassPoints(seg[i], seg[i+1], *blk, cruise_alt);
                for (const auto& bp : bypass) ns.push_back(bp);
                redo = true;
            }
            ns.push_back(seg[i+1]);
        }
        seg = ns;
    }
    return seg;
}

// ingress_bearing_offset_rad 由 GA 给出（±60° 范围内的任意值，不再是硬编码的均分）
static void generateCruise(UAVPath& path, const GeoPoint& start, const GeoPoint& target,
                            double cruise_alt, double spacing_m,
                            const std::vector<NoFlyZone>& zones,
                            double search_radius,
                            double ingress_bearing_offset_rad,
                            double cruise_speed_mps) {
    GeoPoint from = start;
    from.alt = cruise_alt;
    GeoPoint to = {target.lon, target.lat, cruise_alt};

    // 入场锚点：由 GA 决定方位偏移，几何上仍贴着搜索环外缘
    std::vector<GeoPoint> anchors = {from};
    bool hasAnchor = std::abs(ingress_bearing_offset_rad) > 1e-3;
    if (hasAnchor) {
        double directBearing = geo::bearing(from, to);
        double approachFromTarget = directBearing + M_PI + ingress_bearing_offset_rad;
        double totalDist = geo::haversineDistance(from, to);
        double ingressDistBase = std::min(totalDist * 0.6, search_radius + 2000.0);

        const double shrink[] = {1.0, 0.8, 0.6, 0.4};
        for (double s : shrink) {
            GeoPoint candidate = geo::destinationPoint(to, approachFromTarget, ingressDistBase * s);
            candidate.alt = cruise_alt;
            bool inZone = false;
            for (const auto& z : zones) {
                if (cruise_alt < z.min_alt) continue;
                if (z.max_alt > 0 && cruise_alt > z.max_alt) continue;
                if (isPointInNoFlyZone(candidate, z, 1.05)) { inZone = true; break; }
            }
            if (!inZone) { anchors.push_back(candidate); break; }
        }
    }
    anchors.push_back(to);

    // 逐段 A* 求解，拼接为完整折线
    std::vector<GeoPoint> route_points = {anchors[0]};
    for (size_t i = 0; i + 1 < anchors.size(); ++i) {
        auto seg = solveSegment(anchors[i], anchors[i+1], cruise_alt, spacing_m, zones);
        for (size_t k = 1; k < seg.size(); ++k) route_points.push_back(seg[k]);
    }

    // 直接输出 A* 视线平滑后的关键折点，不再按间距插值
    for (size_t i = 1; i < route_points.size(); ++i) {
        GeoPoint wp = route_points[i];
        wp.alt = cruise_alt;
        wp.speed_mps = cruise_speed_mps;
        path.waypoints.push_back(wp);
        path.phase_labels.push_back("cruise");
    }
}

// ============================================================
// 阶段 3：搜索 —— 螺旋模式
// 1.25 圈内收 Archimedean 螺旋，24 散点。
// r(t) = R·(1 − 0.4·t/tMax)，半径从 R 收敛至 0.6R。
// 各机起始方位按 2π/N 均分错开；高度在 cruise_alt ±10% 正弦起伏。
// ============================================================
static void generateSearch_Spiral(UAVPath& path, const GeoPoint& target,
                                   double cruise_alt, double search_radius,
                                   int uav_index, int uav_count,
                                   double cruise_speed_mps) {
    const int totalPoints = 6;
    const double tMax = 2.0 * M_PI * 1.25;
    double altMin = cruise_alt * 0.9;
    double altMax = cruise_alt * 1.1;
    double phaseOffset = (uav_count > 0) ? (2.0 * M_PI * uav_index / uav_count) : 0.0;

    for (int i = 0; i < totalPoints; ++i) {
        double t = tMax * i / (totalPoints - 1);
        double r = search_radius * (1.0 - 0.4 * t / tMax);
        GeoPoint wp = geo::destinationPoint(target, phaseOffset + t, r);
        wp.alt = (altMin + altMax) * 0.5
               + (altMax - altMin) * 0.5 * std::sin(2.0 * M_PI * i / totalPoints);
        wp.speed_mps = cruise_speed_mps * 0.5;
        path.waypoints.push_back(wp);
        path.phase_labels.push_back("search");
    }
}

// ============================================================
// 阶段 3：搜索 —— 8 字模式
// 以目标为交叉点，两叶各飞一整圈（半径 search_radius/2），共 24 散点。
// 轴方位按 π/N·uav_index 错开，多机 8 字朝向各不相同。
// 高度在 cruise_alt ±10% 正弦起伏，跨越两叶各完整一次。
// ============================================================
static void generateSearch_Figure8(UAVPath& path, const GeoPoint& target,
                                    double cruise_alt, double search_radius,
                                    int uav_index, int uav_count,
                                    double cruise_speed_mps) {
    const int pointsPerLobe = 3;    // 每叶 3 点，合计 6 点
    const double lobeR = search_radius / 2.0;
    double altMin = cruise_alt * 0.9;
    double altMax = cruise_alt * 1.1;

    // 两叶轴方位：各机在 [0, π) 内均分，使多机 8 字朝向不重叠
    double axisBearing = (uav_count > 0)
        ? (M_PI * uav_index / uav_count)
        : 0.0;

    // 两叶圆心：沿轴向目标两侧各偏 lobeR
    GeoPoint center1 = geo::destinationPoint(target, axisBearing,          lobeR);
    GeoPoint center2 = geo::destinationPoint(target, axisBearing + M_PI,   lobeR);
    center1.alt = cruise_alt;
    center2.alt = cruise_alt;

    // 叶 1：绕 center1 顺时针一圈，起点 = 目标（从 center1 出发反向即为目标方向）
    // 目标相对于 center1 的方位 = axisBearing + π
    double startAngle1 = axisBearing + M_PI;
    for (int i = 0; i < pointsPerLobe; ++i) {
        double t = 2.0 * M_PI * i / pointsPerLobe;
        GeoPoint wp = geo::destinationPoint(center1, startAngle1 + t, lobeR);
        double altPhase = (double)i / (2 * pointsPerLobe);
        wp.alt = (altMin + altMax) * 0.5
               + (altMax - altMin) * 0.5 * std::sin(2.0 * M_PI * altPhase);
        wp.speed_mps = cruise_speed_mps * 0.5;
        path.waypoints.push_back(wp);
        path.phase_labels.push_back("search");
    }

    double startAngle2 = axisBearing;
    for (int i = 0; i < pointsPerLobe; ++i) {
        double t = 2.0 * M_PI * i / pointsPerLobe;
        GeoPoint wp = geo::destinationPoint(center2, startAngle2 + t, lobeR);
        double altPhase = 0.5 + (double)i / (2 * pointsPerLobe);
        wp.alt = (altMin + altMax) * 0.5
               + (altMax - altMin) * 0.5 * std::sin(2.0 * M_PI * altPhase);
        wp.speed_mps = cruise_speed_mps * 0.5;
        path.waypoints.push_back(wp);
        path.phase_labels.push_back("search");
    }
}

// 统一入口：按 pattern 分发
static void generateSearch(UAVPath& path, const GeoPoint& target,
                           double cruise_alt, double search_radius,
                           int uav_index, int uav_count,
                           SearchPattern pattern, double cruise_speed_mps) {
    switch (pattern) {
        case SearchPattern::FIGURE8:
            generateSearch_Figure8(path, target, cruise_alt, search_radius,
                                   uav_index, uav_count, cruise_speed_mps);
            break;
        case SearchPattern::SPIRAL:
        default:
            generateSearch_Spiral(path, target, cruise_alt, search_radius,
                                  uav_index, uav_count, cruise_speed_mps);
            break;
    }
}

// ============================================================
// 阶段 4：打击（strike）
// 45°～60° 斜切弹道（固定翼气动限制下垂直俯冲会失控）
//   1) 若水平距 < altDrop/tan60°，沿回撤方位延伸接敌点
//   2) 若水平距 > altDrop/tan45°，先平飞消化多余水平距
//   3) 从接敌点沿直线等分斜切至目标，俯冲角恒定约 50°
// ============================================================
static void generateStrike(UAVPath& path, const GeoPoint& target,
                           double spacing_m, double cruise_speed_mps) {
    if (path.waypoints.empty()) return;

    GeoPoint lastWp = path.waypoints.back();
    double altDrop = lastWp.alt - target.alt;

    if (altDrop <= 1.0) {
        int steps = 2;
        for (int i = 1; i <= steps; ++i) {
            double t = (double)i / steps;
            GeoPoint wp = geo::lerp(lastWp, target, t);
            wp.speed_mps = cruise_speed_mps * (1.0 + 0.5 * t); // 小角度也线性加速
            path.waypoints.push_back(wp);
            path.phase_labels.push_back("strike");
        }
        return;
    }

    double hDist = geo::haversineDistance(lastWp, target);
    const double minH = altDrop * 0.5774;
    const double maxH = altDrop * 1.0000;
    const double aimH = altDrop * 0.8391;

    GeoPoint diveStart = lastWp;

    if (hDist < minH) {
        double backBearing = geo::bearing(target, lastWp);
        diveStart = geo::destinationPoint(target, backBearing, aimH);
        diveStart.alt = lastWp.alt;
        double approachD = geo::haversineDistance(lastWp, diveStart);
        int aSteps = std::max(1, (int)(approachD / (spacing_m * 3.0)));
        for (int i = 1; i <= aSteps; ++i) {
            double t = (double)i / aSteps;
            GeoPoint wp = geo::lerp(lastWp, diveStart, t);
            wp.alt = lastWp.alt;
            wp.speed_mps = cruise_speed_mps;
            path.waypoints.push_back(wp);
            path.phase_labels.push_back("strike");
        }
    } else if (hDist > maxH) {
        double fwdBearing = geo::bearing(lastWp, target);
        double levelD = hDist - maxH;
        GeoPoint levelEnd = geo::destinationPoint(lastWp, fwdBearing, levelD);
        levelEnd.alt = lastWp.alt;
        int lSteps = std::max(1, (int)(levelD / (spacing_m * 3.0)));
        for (int i = 1; i <= lSteps; ++i) {
            double t = (double)i / lSteps;
            GeoPoint wp = geo::lerp(lastWp, levelEnd, t);
            wp.alt = lastWp.alt;
            wp.speed_mps = cruise_speed_mps;
            path.waypoints.push_back(wp);
            path.phase_labels.push_back("strike");
        }
        diveStart = levelEnd;
    }

    // 俯冲段：速度从 cruise 线性加速到 1.5×cruise（重力加速）
    int diveSteps = 3;
    for (int i = 1; i <= diveSteps; ++i) {
        double t = (double)i / diveSteps;
        GeoPoint wp = geo::lerp(diveStart, target, t);
        wp.speed_mps = cruise_speed_mps * (1.0 + 0.5 * t);
        path.waypoints.push_back(wp);
        path.phase_labels.push_back("strike");
    }
}

// ============================================================
// 冲突消解
// ============================================================

// 对 approach（cruise）和 attack（search）路径施加相同高度偏移；打击段保持俯冲几何不变
static void applyCruiseAltOffset(UAVPath& approachPath, UAVPath& attackPath, double offset_m) {
    for (size_t i = 0; i < approachPath.waypoints.size(); ++i)
        if (approachPath.phase_labels[i] == "cruise")
            approachPath.waypoints[i].alt += offset_m;
    for (size_t i = 0; i < attackPath.waypoints.size(); ++i)
        if (attackPath.phase_labels[i] == "search")
            attackPath.waypoints[i].alt += offset_m;
}

// 冲突消解：
// 不再使用原地悬停堆叠（旧版 addHoverDelay 会产生视觉上的「等待环」）。
// 策略改为：对低优先级机施加逐轮增大的巡航+搜索高度偏移（60m / 100m / 140m），
// 奇偶 UAV 分别向上/向下，形成立体分层。时序差异由巡航段多方向入场
// 自然带来的路径长度差产生，无需强制悬停。
static std::string resolveConflicts(std::vector<UAVPath>& approachPaths,
                                     std::vector<UAVPath>& attackPaths,
                                     double separation_m) {
    size_t n = approachPaths.size();
    std::string unresolved;
    for (int iter = 0; iter < 3; ++iter) {
        bool anyConflict = false;
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                UAVPath mi = mergePaths(approachPaths[i], attackPaths[i]);
                UAVPath mj = mergePaths(approachPaths[j], attackPaths[j]);
                if (hasConflict(mi, mj, separation_m, 1.0)) {
                    anyConflict = true;
                    double mag = 60.0 + 40.0 * iter;
                    double dir = (j % 2 == 0) ? 1.0 : -1.0;
                    applyCruiseAltOffset(approachPaths[j], attackPaths[j], mag * dir);
                }
            }
        }
        if (!anyConflict) break;
        if (iter == 2) {
            for (size_t i = 0; i < n; ++i)
                for (size_t j = i + 1; j < n; ++j) {
                    UAVPath mi = mergePaths(approachPaths[i], attackPaths[i]);
                    UAVPath mj = mergePaths(approachPaths[j], attackPaths[j]);
                    if (hasConflict(mi, mj, separation_m, 1.0)) {
                        if (!unresolved.empty()) unresolved += "; ";
                        unresolved += "UAV" + std::to_string(approachPaths[i].uav_id) +
                                      "-UAV" + std::to_string(approachPaths[j].uav_id);
                    }
                }
        }
    }
    return unresolved;
}

// ============================================================
// 多边形辅助（打击区域）
// 所有计算在以多边形质心为原点的 equirectangular 米制平面上进行
// ============================================================

struct Vec2 { double x, y; };

static Vec2 geoToLocal(const GeoPoint& p, const GeoPoint& origin, double cosLat0) {
    return {
        (p.lon - origin.lon) * geo::DEG_TO_RAD * cosLat0 * geo::EARTH_RADIUS_M,
        (p.lat - origin.lat) * geo::DEG_TO_RAD * geo::EARTH_RADIUS_M
    };
}

static GeoPoint localToGeo(Vec2 v, const GeoPoint& origin, double cosLat0) {
    GeoPoint r;
    r.lon = origin.lon + v.x / (cosLat0 * geo::EARTH_RADIUS_M) * geo::RAD_TO_DEG;
    r.lat = origin.lat + v.y / geo::EARTH_RADIUS_M * geo::RAD_TO_DEG;
    r.alt = origin.alt;
    return r;
}

// 简单多边形质心（顶点平均）
static GeoPoint polygonCentroid(const std::vector<GeoPoint>& poly) {
    GeoPoint c{0, 0, 0};
    for (const auto& p : poly) { c.lon += p.lon; c.lat += p.lat; c.alt += p.alt; }
    double n = (double)poly.size();
    c.lon /= n; c.lat /= n; c.alt /= n;
    return c;
}

// 射线法判断点是否在多边形内部（2D，米制坐标）
static bool pointInPolygon(Vec2 pt, const std::vector<Vec2>& poly) {
    bool inside = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if (((poly[i].y > pt.y) != (poly[j].y > pt.y)) &&
            (pt.x < (poly[j].x - poly[i].x) * (pt.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
            inside = !inside;
    }
    return inside;
}

// 从原点沿 bearing 发射射线，求到多边形边的最近交点距离
// 返回 -1 表示无交点（不应发生于凸/简单多边形内部点）
static double rayToPolygonEdge(Vec2 origin, double bearing_rad,
                                const std::vector<Vec2>& poly) {
    double dx = std::sin(bearing_rad);
    double dy = std::cos(bearing_rad);
    double minT = 1e18;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        double ex = poly[i].x - poly[j].x;
        double ey = poly[i].y - poly[j].y;
        double denom = dx * ey - dy * ex;
        if (std::abs(denom) < 1e-12) continue;
        double t = ((poly[j].x - origin.x) * ey - (poly[j].y - origin.y) * ex) / denom;
        double u = ((poly[j].x - origin.x) * dy - (poly[j].y - origin.y) * dx) / denom;
        if (t > 1e-6 && u >= 0.0 && u <= 1.0 && t < minT) minT = t;
    }
    return (minT < 1e17) ? minT : -1.0;
}

// 在多边形内部分配 N 个打击点：
// 从质心沿 N 个均分方位射线，各取到边界距离的 65%
// 若某点落在外部（凹多边形极端情况），向质心拉回
static std::vector<GeoPoint> distributeStrikePoints(
    const std::vector<GeoPoint>& poly, int n, const GeoPoint& centroid)
{
    double cosLat0 = std::cos(centroid.lat * geo::DEG_TO_RAD);
    std::vector<Vec2> polyLocal(poly.size());
    for (size_t i = 0; i < poly.size(); ++i)
        polyLocal[i] = geoToLocal(poly[i], centroid, cosLat0);

    Vec2 cLocal = {0, 0}; // centroid 就是原点

    std::vector<GeoPoint> pts(n);
    if (n == 1) {
        pts[0] = centroid;
        return pts;
    }

    for (int i = 0; i < n; ++i) {
        double angle = 2.0 * M_PI * i / n;
        double edgeDist = rayToPolygonEdge(cLocal, angle, polyLocal);
        double r = (edgeDist > 0) ? edgeDist * 0.65 : 500.0;
        Vec2 pt = {r * std::sin(angle), r * std::cos(angle)};
        // 若凹多边形导致落在外部，向质心拉回
        if (!pointInPolygon(pt, polyLocal)) {
            for (double shrink = 0.5; shrink > 0.05; shrink -= 0.1) {
                pt = {r * shrink * std::sin(angle), r * shrink * std::cos(angle)};
                if (pointInPolygon(pt, polyLocal)) break;
            }
        }
        pts[i] = localToGeo(pt, centroid, cosLat0);
        pts[i].alt = centroid.alt;
    }
    return pts;
}

// 多边形外接圆半径（质心到最远顶点距离）
static double polygonOuterRadius(const std::vector<GeoPoint>& poly, const GeoPoint& centroid) {
    double maxR = 0;
    for (const auto& v : poly) {
        double d = geo::haversineDistance(centroid, v);
        if (d > maxR) maxR = d;
    }
    return maxR;
}

// ============================================================
// 路径构建（由 planPaths 和优化器适应度评估共用）
// params 提供各机的入场角和高度额外偏移
// ============================================================
static void buildPaths(const PlanningInput& input,
                        const std::vector<GeoPoint>& strikePoints,
                        bool isAreaTarget, double areaEquivR,
                        const MissionParams& params,
                        std::vector<UAVPath>& approachPaths,
                        std::vector<UAVPath>& attackPaths) {
    approachPaths.clear();
    attackPaths.clear();
    int n = input.uav_count;

    for (int i = 0; i < n; ++i) {
        const GeoPoint& myTarget = strikePoints[i];
        // cruise_alt_max > cruise_alt：各机在 [cruise_alt, cruise_alt_max] 均匀分配
        // 否则：固定 cruise_alt + ±20m 等差错层（原有逻辑）
        double baseCruiseAlt;
        if (input.cruise_alt_max > input.cruise_alt) {
            baseCruiseAlt = (n > 1)
                ? input.cruise_alt + (input.cruise_alt_max - input.cruise_alt) * i / (n - 1)
                : (input.cruise_alt + input.cruise_alt_max) * 0.5;
        } else {
            double altStagger = (n > 1) ? (i - (n - 1) / 2.0) * 20.0 : 0.0;
            baseCruiseAlt = input.cruise_alt + altStagger;
        }
        double uavCruiseAlt = baseCruiseAlt + params.alt_offsets[i];

        UAVPath ap; ap.uav_id = i;
        generateTakeoff(ap, input.start_positions[i], myTarget,
                        uavCruiseAlt, input.waypoint_spacing_m, input.cruise_speed_mps,
                        input.no_fly_zones);
        GeoPoint takeoffEnd = ap.waypoints.back();
        generateCruise(ap, takeoffEnd, myTarget, uavCruiseAlt, input.waypoint_spacing_m,
                       input.no_fly_zones, input.search_radius,
                       params.ingress_angles[i], input.cruise_speed_mps);

        UAVPath atk; atk.uav_id = i;
        double effSearchR = input.search_radius;
        if (isAreaTarget && n > 1)
            effSearchR = std::max(input.search_radius, areaEquivR / std::sqrt((double)n));
        generateSearch(atk, myTarget, uavCruiseAlt, effSearchR,
                       i, n, input.search_pattern, input.cruise_speed_mps);
        generateStrike(atk, myTarget, input.waypoint_spacing_m, input.cruise_speed_mps);

        // 过渡段：将巡航末端（目标中心）连接到搜索入场点（外圈），
        // 避免路径在可视化中出现 search_radius 量级的跳变
        if (!ap.waypoints.empty() && !atk.waypoints.empty()) {
            GeoPoint apEnd   = ap.waypoints.back();
            GeoPoint atkStart = atk.waypoints.front();
            double transitDist = geo::haversineDistance(apEnd, atkStart);
            if (transitDist > input.waypoint_spacing_m * 0.5) {
                int steps = std::max(1, (int)(transitDist / (input.waypoint_spacing_m * 10.0)));
                for (int k = 1; k <= steps; ++k) {
                    double frac = (double)k / steps;
                    GeoPoint wp = geo::lerp(apEnd, atkStart, frac);
                    wp.alt = uavCruiseAlt;
                    wp.speed_mps = input.cruise_speed_mps;
                    ap.waypoints.push_back(wp);
                    ap.phase_labels.push_back("cruise");
                }
            }
        }

        approachPaths.push_back(ap);
        attackPaths.push_back(atk);
    }
}

// ============================================================
// 遗传算法实现
// 染色体编码：长度 2N 的实数向量
//   基因 [0, N)   = 各机入场角偏移（弧度），范围 ±ingress_range
//   基因 [N, 2N)  = 各机高度额外偏移（米），范围 ±alt_range
// ============================================================
#include <random>

GeneticOptimizer::GeneticOptimizer(GAConfig cfg) : cfg_(cfg) {}

MissionParams GeneticOptimizer::optimize(
    const PlanningInput& input,
    const std::function<double(const MissionParams&)>& fitness_fn)
{
    int n = input.uav_count;
    int geneLen = 2 * n;

    // 随机数引擎：seed=0 时使用随机设备
    std::mt19937 rng(cfg_.seed != 0 ? (unsigned)cfg_.seed
                                     : std::random_device{}());
    std::uniform_real_distribution<double> unif01(0.0, 1.0);
    std::normal_distribution<double>       gauss(0.0, 1.0);

    // 基因值上下界
    auto clamp = [&](std::vector<double>& g) {
        for (int i = 0; i < n; ++i) {
            g[i]     = std::max(-cfg_.ingress_range, std::min(cfg_.ingress_range, g[i]));
            g[n + i] = std::max(-cfg_.alt_range,     std::min(cfg_.alt_range,     g[n + i]));
        }
    };

    // 染色体 → MissionParams
    auto toParams = [&](const std::vector<double>& g) {
        MissionParams p;
        p.ingress_angles.assign(g.begin(),     g.begin() + n);
        p.alt_offsets.assign(  g.begin() + n,  g.end());
        return p;
    };

    // 随机初始个体
    auto makeGenes = [&]() {
        std::vector<double> g(geneLen);
        std::uniform_real_distribution<double> ia(-cfg_.ingress_range, cfg_.ingress_range);
        std::uniform_real_distribution<double> ao(-cfg_.alt_range,     cfg_.alt_range);
        for (int i = 0; i < n; ++i) { g[i] = ia(rng); g[n + i] = ao(rng); }
        return g;
    };

    using Chromo = std::pair<std::vector<double>, double>; // <genes, fitness>
    auto cmp = [](const Chromo& a, const Chromo& b) { return a.second < b.second; };

    // 初始化种群
    std::vector<Chromo> pop(cfg_.population_size);
    for (auto& c : pop) {
        c.first  = makeGenes();
        c.second = fitness_fn(toParams(c.first));
    }
    std::sort(pop.begin(), pop.end(), cmp);

    // 锦标赛选择（k=3）
    auto tournament = [&]() -> const std::vector<double>& {
        std::uniform_int_distribution<int> idx(0, cfg_.population_size - 1);
        int best = idx(rng);
        for (int k = 1; k < 3; ++k) {
            int c = idx(rng);
            if (pop[c].second < pop[best].second) best = c;
        }
        return pop[best].first;
    };

    for (int gen = 0; gen < cfg_.generations; ++gen) {
        std::vector<Chromo> next;
        next.reserve(cfg_.population_size);

        // 精英直接保留
        int elite = std::min(cfg_.elite_count, cfg_.population_size);
        for (int i = 0; i < elite; ++i) next.push_back(pop[i]);

        // 交叉 + 变异填充剩余名额
        while ((int)next.size() < cfg_.population_size) {
            const auto& p1 = tournament();
            const auto& p2 = tournament();
            std::vector<double> child(geneLen);

            if (unif01(rng) < cfg_.crossover_rate) {
                // 均匀交叉
                for (int k = 0; k < geneLen; ++k)
                    child[k] = (unif01(rng) < 0.5) ? p1[k] : p2[k];
            } else {
                child = p1;
            }

            // 高斯变异：sigma = 搜索范围的 20%
            for (int k = 0; k < geneLen; ++k) {
                if (unif01(rng) < cfg_.mutation_rate) {
                    double sigma = (k < n) ? cfg_.ingress_range * 0.2
                                           : cfg_.alt_range     * 0.2;
                    child[k] += gauss(rng) * sigma;
                }
            }
            clamp(child);

            next.push_back({child, fitness_fn(toParams(child))});
        }

        pop = std::move(next);
        std::sort(pop.begin(), pop.end(), cmp);
    }

    return toParams(pop[0].first);
}

// ============================================================
// 粒子群优化器（PSO）实现
// 粒子编码与 GA 相同：[ingress_0..N-1, alt_offset_0..N-1]
// 惯性权重随迭代线性递减（w: inertia_w → min_w），避免早熟收敛
// ============================================================
PSOOptimizer::PSOOptimizer(PSOConfig cfg) : cfg_(cfg) {}

MissionParams PSOOptimizer::optimize(
    const PlanningInput& input,
    const std::function<double(const MissionParams&)>& fitness_fn)
{
    int n = input.uav_count;
    int dim = 2 * n;  // [ingress×N, alt_offset×N]

    std::mt19937 rng(cfg_.seed != 0 ? (unsigned)cfg_.seed : std::random_device{}());
    std::uniform_real_distribution<double> unif01(0.0, 1.0);

    // 各维度的搜索范围
    auto lo = [&](int d) { return d < n ? -cfg_.ingress_range : -cfg_.alt_range; };
    auto hi = [&](int d) { return d < n ?  cfg_.ingress_range :  cfg_.alt_range; };

    // 最大速度 = 范围 × 0.5（防止粒子飞出边界）
    auto vmax = [&](int d) { return (hi(d) - lo(d)) * 0.5; };

    auto clamp = [](double v, double l, double h) {
        return v < l ? l : (v > h ? h : v);
    };

    auto toParams = [&](const std::vector<double>& x) {
        MissionParams p;
        p.ingress_angles.assign(x.begin(),     x.begin() + n);
        p.alt_offsets.assign(  x.begin() + n,  x.end());
        return p;
    };

    // 初始化粒子：位置均匀随机，速度为零（让认知/社会力自然建立方向）
    struct Particle {
        std::vector<double> pos, vel, pbest;
        double pbest_fit;
    };
    std::vector<Particle> swarm(cfg_.swarm_size);
    std::vector<double>   gbest;
    double                gbest_fit = std::numeric_limits<double>::infinity();

    for (auto& p : swarm) {
        p.pos.resize(dim); p.vel.resize(dim, 0.0);
        for (int d = 0; d < dim; ++d)
            p.pos[d] = lo(d) + unif01(rng) * (hi(d) - lo(d));
        p.pbest     = p.pos;
        p.pbest_fit = fitness_fn(toParams(p.pos));
        if (p.pbest_fit < gbest_fit) { gbest_fit = p.pbest_fit; gbest = p.pos; }
    }

    // 主迭代
    for (int iter = 0; iter < cfg_.iterations; ++iter) {
        // 惯性权重线性递减
        double w = cfg_.inertia_w - (cfg_.inertia_w - cfg_.min_w)
                   * (double)iter / cfg_.iterations;

        for (auto& p : swarm) {
            for (int d = 0; d < dim; ++d) {
                double r1 = unif01(rng), r2 = unif01(rng);
                // 速度更新
                p.vel[d] = w * p.vel[d]
                         + cfg_.c1 * r1 * (p.pbest[d] - p.pos[d])
                         + cfg_.c2 * r2 * (gbest[d]   - p.pos[d]);
                // 限速
                p.vel[d] = clamp(p.vel[d], -vmax(d), vmax(d));
                // 位置更新 + 边界夹紧
                p.pos[d] = clamp(p.pos[d] + p.vel[d], lo(d), hi(d));
            }
            double fit = fitness_fn(toParams(p.pos));
            if (fit < p.pbest_fit) { p.pbest_fit = fit; p.pbest = p.pos; }
            if (fit < gbest_fit)   { gbest_fit   = fit; gbest   = p.pos; }
        }
    }
    return toParams(gbest);
}

// ============================================================
// 主规划函数
// ============================================================
PlanningResult planPaths(const PlanningInput& input, MissionOptimizer* optimizer) {
    PlanningResult result;
    result.success = true;

    if (input.uav_count < 1) {
        result.success = false;
        result.error_msg = "uav_count must be >= 1";
        return result;
    }
    if ((int)input.start_positions.size() < input.uav_count) {
        result.success = false;
        result.error_msg = "Insufficient start positions for uav_count";
        return result;
    }
    if (input.target_area.empty()) {
        result.success = false;
        result.error_msg = "target_area must have at least 1 point";
        return result;
    }

    const bool isAreaTarget = input.target_area.size() >= 4;
    GeoPoint centroid = isAreaTarget
        ? polygonCentroid(input.target_area)
        : input.target_area[0];

    std::vector<GeoPoint> strikePoints;
    if (isAreaTarget && input.uav_count > 1) {
        strikePoints = distributeStrikePoints(input.target_area, input.uav_count, centroid);
    } else {
        strikePoints.assign(input.uav_count, centroid);
    }

    double areaEquivR = isAreaTarget ? polygonOuterRadius(input.target_area, centroid) : 0.0;

    // 默认入场参数（与旧版行为完全一致）
    MissionParams params;
    params.ingress_angles.resize(input.uav_count, 0.0);
    params.alt_offsets.resize(input.uav_count, 0.0);
    if (!isAreaTarget && input.uav_count > 1) {
        for (int i = 0; i < input.uav_count; ++i)
            params.ingress_angles[i] = ((double)i - (input.uav_count - 1) / 2.0)
                                     * (M_PI / 3.0) / (input.uav_count - 1);
    }

    // 有优化器时：用适应度函数驱动算法搜索最优参数
    if (optimizer) {
        result.optimizer_name = optimizer->name();

        // 适应度函数：路径总长 + 冲突惩罚 + 禁飞区违规惩罚
        // 权重硬编码在此处，与具体优化算法无关
        auto fitness_fn = [&](const MissionParams& p) -> double {
            std::vector<UAVPath> ap, atk;
            buildPaths(input, strikePoints, isAreaTarget, areaEquivR, p, ap, atk);

            double totalKm = 0.0;
            int conflicts = 0, nfzViol = 0;

            for (int i = 0; i < input.uav_count; ++i) {
                for (size_t j = 1; j < ap[i].waypoints.size(); ++j)
                    totalKm += geo::distance3D(ap[i].waypoints[j-1], ap[i].waypoints[j]) / 1000.0;
                for (size_t j = 1; j < atk[i].waypoints.size(); ++j)
                    totalKm += geo::distance3D(atk[i].waypoints[j-1], atk[i].waypoints[j]) / 1000.0;

                // 跳过 takeoff，与 planPaths 末尾校验保持一致
                std::vector<GeoPoint> cruisePath;
                for (size_t j = 0; j < ap[i].waypoints.size(); ++j)
                    if (ap[i].phase_labels[j] != "takeoff")
                        cruisePath.push_back(ap[i].waypoints[j]);
                for (const auto& w : atk[i].waypoints) cruisePath.push_back(w);
                UAVPath mi = mergePaths(ap[i], atk[i]);
                if (!isPathValid(cruisePath, input.no_fly_zones)) ++nfzViol;

                for (int j = i + 1; j < input.uav_count; ++j) {
                    UAVPath mj = mergePaths(ap[j], atk[j]);
                    if (hasConflict(mi, mj, 200.0, 1.0)) ++conflicts;
                }
            }
            return totalKm + 50.0 * conflicts + 200.0 * nfzViol;
        };

        params = optimizer->optimize(input, fitness_fn);
    }

    // 用最终参数构建路径
    buildPaths(input, strikePoints, isAreaTarget, areaEquivR, params,
               result.approach_paths, result.attack_paths);

    // 冲突消解（对 GA 优化后的路径做最后一轮兜底）
    if (input.uav_count > 1) {
        std::string unresolved = resolveConflicts(result.approach_paths, result.attack_paths, 200.0);
        if (!unresolved.empty())
            result.error_msg = "Unresolved conflicts: " + unresolved;
    }

    // 起飞阶段无 NFZ 规避（已知限制），只校验巡航及之后的阶段
    for (int i = 0; i < input.uav_count; ++i) {
        std::vector<GeoPoint> checkPath;
        const auto& ap = result.approach_paths[i];
        for (size_t j = 0; j < ap.waypoints.size(); ++j)
            if (ap.phase_labels[j] != "takeoff")
                checkPath.push_back(ap.waypoints[j]);
        for (const auto& w : result.attack_paths[i].waypoints)
            checkPath.push_back(w);
        if (!isPathValid(checkPath, input.no_fly_zones)) {
            if (!result.error_msg.empty()) result.error_msg += "; ";
            result.error_msg += "UAV" + std::to_string(i) + " path violates no-fly zone";
        }
    }

    return result;
}
