//
// Created by Administrator on 2026/5/7.
//

// You may need to build the project (run Qt uic code generator) to get "ui_RoutePlanning.h" resolved

#include "RoutePlanning.h"
#include "ui_RoutePlanning.h"
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QMap>
#include <cmath>


RoutePlanning::RoutePlanning(QWidget *parent) : QFrame(parent), ui(new Ui::RoutePlanning)
{
    ui->setupUi(this);
    applyTechStyle();

    connect(ui->pushButton, &QPushButton::clicked, this, &RoutePlanning::onPlanAllClicked);
}

RoutePlanning::~RoutePlanning()
{
    delete ui;
}

void RoutePlanning::setAllocationData(const QList<UavAssignment> &assignments, int totalUavCount)
{
    m_assignments = assignments;
    m_totalUavCount = totalUavCount;

    // 清空表格并显示分配摘要
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->clearContents();

    // 表格中显示已分配无人机总数
    int assignedCount = m_assignments.size();
    ui->pushButton->setText(QString("一键自动规划  (%1 架 UAV)").arg(assignedCount));
}

// ============================================================
// "一键自动规划" 按钮点击
// ============================================================
void RoutePlanning::onPlanAllClicked()
{
    if (m_assignments.isEmpty()) {
        return;
    }

    // 清空表格
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->clearContents();

    // 按目标分组 UAV
    QMap<QString, QList<UavAssignment>> groups;  // key = targetId
    for (const auto &a : m_assignments)
        groups[a.targetId].append(a);

    // 基准坐标（参考 main_demo.cpp 使用北京周边区域）
    const double baseLon = 116.35;
    const double baseLat = 39.95;

    int assignedRowCount = 0;

    // 遍历每个目标分组进行航路规划
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QList<UavAssignment> &group = it.value();
        if (group.isEmpty()) continue;

        // --- 构建 start_positions ---
        std::vector<GeoPoint> startPositions;
        for (int i = 0; i < group.size(); ++i) {
            // 在基准点附近散布起飞点（每架UAV偏移微小量）
            double lonOff = (i % 5) * 0.008;
            double latOff = (i / 5) * 0.006;
            startPositions.push_back({
                baseLon + lonOff,
                baseLat + latOff + 0.08,
                50.0 + (i % 7) * 2.0
            });
        }

        // --- 构建 target_area ---
        std::vector<GeoPoint> targetArea;
        if (group.first().targetType == "AR") {
            // 区域目标：4 个点组成一个矩形区域
            double tLon = baseLon + 0.12;
            double tLat = baseLat - 0.06;
            targetArea.push_back({tLon, tLat, 30.0});
            targetArea.push_back({tLon + 0.05, tLat, 30.0});
            targetArea.push_back({tLon + 0.05, tLat - 0.04, 30.0});
            targetArea.push_back({tLon, tLat - 0.04, 30.0});
        } else {
            // 点目标：单个坐标点
            double tLon = baseLon + 0.15;
            double tLat = baseLat - 0.10;
            targetArea.push_back({tLon, tLat, 30.0});
        }

        // 构建 PlanningInput 并调用规划算法
        PlanningInput input = buildPlanningInput(group, startPositions, targetArea);
        PlanningResult result = planPaths(input);

        // 将本组结果填入表格
        populateTable(result, group.first().targetName);
        assignedRowCount += group.size();
    }

    ui->pushButton->setText(QString("航路规划完成  (%1 架)").arg(assignedRowCount));
}

// ============================================================
// 根据 UI 控件参数构建规划输入
// ============================================================
PlanningInput RoutePlanning::buildPlanningInput(const QList<UavAssignment> &group,
                                                 const std::vector<GeoPoint> &startPositions,
                                                 const std::vector<GeoPoint> &targetArea)
{
    PlanningInput input;

    input.start_positions = startPositions;
    input.target_area = targetArea;
    input.uav_count = static_cast<int>(group.size());

    // ---- 从 UI 读取参数 ----

    // 爬升高度 → 巡航高度
    input.cruise_alt = parseValue(ui->label_ClimbHeightValue->text());
    if (input.cruise_alt < 100.0) input.cruise_alt = 800.0;

    // 巡航速度 km/h → m/s
    input.cruise_speed_mps = parseSpeed(ui->label_CruiseSpeedValue->text());
    if (input.cruise_speed_mps < 1.0) input.cruise_speed_mps = 60.0;

    // 搜索半径：从搜索高度标签读取
    {
        double sh = parseValue(ui->label_AreaSearchHeightValue->text());
        input.search_radius = (sh > 10.0) ? sh * 4.0 : 2000.0;
    }

    // 散点间距：默认 400m
    input.waypoint_spacing_m = 400.0;

    // 搜索形式 → SearchPattern
    QString searchForm = ui->comboBox_SearchForm->currentText();
    if (searchForm.contains(QString::fromUtf8("8")))
        input.search_pattern = SearchPattern::FIGURE8;
    else
        input.search_pattern = SearchPattern::SPIRAL;

    // 巡航方式 → 影响巡航高度上限
    QString cruiseMode = ui->comboBox_CruiseMode->currentText();
    if (cruiseMode.contains("中空")) {
        input.cruise_alt = 3000.0;
    } else if (cruiseMode.contains("地形跟随")) {
        input.cruise_alt = qMin(input.cruise_alt, 200.0);
    }

    // 禁飞区：默认使用两个示例禁飞区（参考 main_demo.cpp）
    NoFlyZone nfz1;
    nfz1.min_alt = 0;
    nfz1.max_alt = -1;
    nfz1.polygon = {
        {116.31, 40.00, 0.0}, {116.36, 40.01, 0.0}, {116.40, 39.98, 0.0},
        {116.39, 39.93, 0.0}, {116.34, 39.91, 0.0}, {116.29, 39.94, 0.0}
    };

    NoFlyZone nfz2;
    nfz2.min_alt = 0;
    nfz2.max_alt = -1;
    nfz2.polygon = {
        {116.40, 39.99, 0.0}, {116.46, 39.97, 0.0},
        {116.45, 39.92, 0.0}, {116.38, 39.93, 0.0}
    };

    input.no_fly_zones = {nfz1, nfz2};

    return input;
}

// ============================================================
// 将规划结果填入表格
// ============================================================
void RoutePlanning::populateTable(const PlanningResult &result, const QString &targetName)
{
    if (!result.success) return;

    int n = static_cast<int>(result.approach_paths.size());
    int currentRow = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(currentRow + n);

    for (int i = 0; i < n; ++i) {
        int row = currentRow + i;

        const UAVPath &approach = result.approach_paths[i];
        const UAVPath &attack = result.attack_paths[i];

        // 编号
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(row + 1));
        idItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(row, 0, idItem);

        // 无人机名称
        QString uavName;
        for (const auto &a : m_assignments) {
            if (a.uavIndex == approach.uav_id) {
                uavName = a.uavId;
                break;
            }
        }
        if (uavName.isEmpty())
            uavName = QString("UAV-%1").arg(approach.uav_id + 1, 2, 10, QChar('0'));
        QTableWidgetItem *uavItem = new QTableWidgetItem(uavName);
        uavItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(row, 1, uavItem);

        // 目标
        QTableWidgetItem *tgtItem = new QTableWidgetItem(targetName);
        tgtItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(row, 2, tgtItem);

        // 总航程：累加 approach + attack 中每段距离
        double totalDist = 0.0;
        auto calcPathLen = [&](const std::vector<GeoPoint> &pts) {
            for (size_t k = 1; k < pts.size(); ++k)
                totalDist += geo::haversineDistance(pts[k - 1], pts[k]);
        };
        calcPathLen(approach.waypoints);
        calcPathLen(attack.waypoints);

        QTableWidgetItem *distItem = new QTableWidgetItem(
            QString("%1 km").arg(totalDist / 1000.0, 0, 'f', 2));
        distItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(row, 3, distItem);

        // 状态
        QTableWidgetItem *statusItem = new QTableWidgetItem("已生成");
        statusItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(row, 4, statusItem);
    }
}

// ============================================================
// 工具函数：解析 UI 中的数值
// ============================================================
double RoutePlanning::parseValue(const QString &text)
{
    // 提取字符串中的数字部分（如 "800m" → 800, "±3s" → 3, "45°" → 45）
    QString digits;
    for (const QChar &ch : text) {
        if (ch.isDigit() || ch == '.')
            digits.append(ch);
    }
    return digits.toDouble();
}

double RoutePlanning::parseSpeed(const QString &text)
{
    // 解析速度 "320km/h" → 320 → 88.89 m/s
    double kmh = parseValue(text);
    return kmh / 3.6;
}

// ============================================================
// 以下为样式代码（保持不变）
// ============================================================
void RoutePlanning::applyTechStyle()
{
    const QString baseBg = "#0a0e1a";
    const QString panelBg = "#0d1326";
    const QString borderColor = "#1a3a6a";
    const QString accentBlue = "#00b4ff";
    const QString accentCyan = "#00e5ff";
    const QString textPrimary = "#e0e8f0";
    const QString textSecondary = "#7a8ba8";
    const QString inputBg = "#0f1a2e";
    const QString inputBorder = "#1a3a6a";
    const QString hoverBorder = "#00b4ff";
    const QString groupBoxBg = "#0b1124";
    const QString groupBoxTitle = "#00b4ff";

    setStyleSheet(QString(R"(
        RoutePlanning {
            background-color: %1;
            border: 2px solid %2;
            border-radius: 6px;
        }
        QWidget {
            color: %3;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
        }
    )").arg(baseBg).arg(borderColor).arg(textPrimary));

    ui->widget_5->setStyleSheet(QString(R"(
        QWidget#widget_5 {
            background-color: %1;
            border-top: 1px solid %2;
        }
    )").arg(panelBg).arg(borderColor));

    QString phaseWidgetStyle = QString(R"(
        QWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
        }
    )").arg(groupBoxBg).arg(borderColor);
    ui->widget->setStyleSheet(phaseWidgetStyle);
    ui->widget_2->setStyleSheet(phaseWidgetStyle);
    ui->widget_3->setStyleSheet(phaseWidgetStyle);
    ui->widget_4->setStyleSheet(phaseWidgetStyle);

    QString groupBoxTechStyle = QString(R"(
        QGroupBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            margin-top: 20px;
            padding: 14px 8px 8px 8px;
            font-weight: bold;
            color: %3;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 2px 10px;
            color: %3;
            font-size: 12px;
            background-color: %4;
            border: 1px solid %2;
            border-radius: 3px;
            margin-left: 0px;
            margin-top: 6px;
        }
    )").arg(groupBoxBg).arg(borderColor).arg(groupBoxTitle).arg(inputBg);

    for (QGroupBox* gb : findChildren<QGroupBox*>()) {
        gb->setStyleSheet(groupBoxTechStyle);
    }

    QString buttonTechStyle = QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 4px 12px;
            font-size: 12px;
            font-weight: bold;
            min-height: 28px;
        }
        QPushButton:hover {
            background-color: %4;
            border: 1px solid %5;
            color: %6;
        }
        QPushButton:pressed {
            background-color: %7;
            border: 1px solid %5;
        }
    )").arg(inputBg).arg(borderColor).arg(textPrimary)
       .arg("#0f1a3e").arg(hoverBorder).arg(accentCyan)
       .arg("#081020");

    for (QPushButton* btn : findChildren<QPushButton*>()) {
        btn->setStyleSheet(buttonTechStyle);
    }

    ui->pushButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 6px 16px;
            font-size: 13px;
            font-weight: bold;
            min-height: 36px;
        }
        QPushButton:hover {
            background-color: %4;
            border: 1px solid %5;
            color: %6;
        }
        QPushButton:pressed {
            background-color: %7;
        }
    )").arg("#0a1628").arg(accentBlue).arg(accentCyan)
       .arg("#0f1a3e").arg(accentCyan).arg("#ffffff")
       .arg("#060e1a"));

    QString comboBoxStyle = QString(R"(
        QComboBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 2px 8px;
            font-size: 12px;
            min-height: 26px;
        }
        QComboBox:hover {
            border: 1px solid %4;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left: 1px solid %2;
        }
        QComboBox::down-arrow {
            width: 8px;
            height: 8px;
        }
        QComboBox QAbstractItemView {
            background-color: %1;
            border: 1px solid %2;
            color: %3;
            selection-background-color: %5;
            selection-color: %6;
        }
    )").arg(inputBg).arg(borderColor).arg(textPrimary)
       .arg(hoverBorder).arg("#0d2466").arg(accentCyan);

    for (QComboBox* cb : findChildren<QComboBox*>()) {
        cb->setStyleSheet(comboBoxStyle);
    }

    QString tableStyle = QString(R"(
        QTableWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            gridline-color: %3;
            color: %4;
            font-size: 12px;
            selection-background-color: %5;
            selection-color: %6;
        }
        QTableWidget::item {
            padding: 4px 8px;
        }
        QTableWidget::item:alternate {
            background-color: %7;
        }
        QHeaderView::section {
            background-color: %8;
            border: 1px solid %2;
            color: %9;
            font-weight: bold;
            font-size: 12px;
            padding: 6px 8px;
        }
    )").arg(panelBg).arg(borderColor).arg("#1a2a4a")
       .arg(textPrimary).arg("#0d2466").arg("#ffffff")
       .arg("#0f1a2e").arg("#0a1628").arg(accentBlue);

    ui->tableWidget->setStyleSheet(tableStyle);

    QStringList labelNames = {
        "label_ClimbRate", "label_ClimbHeight", "label_TakeoffAzimuth",
        "label_Phase1Index", "label_Phase1Name",
        "label_ClimbHeightValue", "label_TakeoffAzimuthValue",
        "label_Phase2Index", "label_Phase2Name",
        "label_CruiseMode", "label_CruiseSpeed", "label_BypassStrategy",
        "label_CruiseSpeedValue",
        "label_Phase3Index", "label_Phase3Name",
        "label_SearchForm", "label_AreaSearchForm", "label_SearchHeight",
        "label_MaxSearchDuration", "label_AreaSearchHeightValue",
        "label_SearchDurationValue",
        "label_Phase4Index", "label_Phase4Name",
        "label_AttackMode", "label_DiveAngle", "label_SimultaneousArrivalAccuracy",
        "label_AttackAzimuth", "label_DiveAngleValue",
        "label_SimultaneousArrivalAccuracyValue"
    };

    for (const QString& name : labelNames) {
        QLabel* label = findChild<QLabel*>(name);
        if (!label) continue;

        QString color = textSecondary;
        int fontSize = 12;
        bool isBold = false;

        if (name == "label_Phase1Index" || name == "label_Phase2Index" ||
            name == "label_Phase3Index" || name == "label_Phase4Index") {
            color = accentCyan;
            fontSize = 16;
            isBold = true;
        } else if (name == "label_Phase1Name" || name == "label_Phase2Name" ||
                   name == "label_Phase3Name" || name == "label_Phase4Name") {
            color = accentBlue;
            fontSize = 14;
            isBold = true;
        } else if (name == "label_ClimbRate" || name == "label_ClimbHeight" ||
                   name == "label_TakeoffAzimuth" || name == "label_CruiseMode" ||
                   name == "label_CruiseSpeed" || name == "label_BypassStrategy" ||
                   name == "label_SearchForm" || name == "label_AreaSearchForm" ||
                   name == "label_SearchHeight" || name == "label_MaxSearchDuration" ||
                   name == "label_AttackMode" || name == "label_DiveAngle" ||
                   name == "label_SimultaneousArrivalAccuracy" || name == "label_AttackAzimuth") {
            color = textPrimary;
            fontSize = 12;
            isBold = true;
        } else if (name == "label_ClimbHeightValue" || name == "label_TakeoffAzimuthValue" ||
                   name == "label_CruiseSpeedValue" || name == "label_AreaSearchHeightValue" ||
                   name == "label_SearchDurationValue" || name == "label_DiveAngleValue" ||
                   name == "label_SimultaneousArrivalAccuracyValue") {
            color = accentCyan;
            fontSize = 12;
            isBold = true;
        }

        label->setStyleSheet(QString(R"(
            QLabel {
                color: %1;
                font-size: %2px;
                font-weight: %3;
                background: transparent;
                border: none;
                padding: 1px 2px;
            }
        )").arg(color).arg(fontSize).arg(isBold ? "bold" : "normal"));
    }

    // 初始化表格列
    QStringList headers = {"编号", "无人机", "目标", "总航程", "状态"};
    ui->tableWidget->setColumnCount(5);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionBehavior(QTableWidget::SelectRows);
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setEditTriggers(QTableWidget::NoEditTriggers);
}
