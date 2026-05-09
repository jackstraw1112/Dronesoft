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
#include <QTimer>
#include <QCoreApplication>
#include <cmath>


RoutePlanning::RoutePlanning(QWidget *parent) : QFrame(parent), ui(new Ui::RoutePlanning)
{
    ui->setupUi(this);
    applyTechStyle();

    connect(ui->pushButton_Plan, &QPushButton::clicked, this, &RoutePlanning::onPlanAllClicked);
}

RoutePlanning::~RoutePlanning()
{
    delete ui;
}

void RoutePlanning::setAllocationData(const QList<UavAssignment> &assignments, int totalUavCount)
{
    m_assignments = assignments;
    m_totalUavCount = totalUavCount;

    ui->tableWidget_Plan->setRowCount(0);
    ui->tableWidget_Plan->clearContents();

    int assignedCount = m_assignments.size();
    ui->pushButton_Plan->setText(QString("一键自动规划  (%1 架 UAV)").arg(assignedCount));

    // 更新 widget_5 中的统计标签
    QMap<QString, int> targetTypes;
    for (const auto &a : m_assignments)
        targetTypes[a.targetId]++;
    int formationCount = 0;
    for (auto it = targetTypes.constBegin(); it != targetTypes.constEnd(); ++it)
        formationCount += it.value();

    if (ui->label_2) ui->label_2->setText(QString::number(formationCount));
    if (ui->label_4) ui->label_4->setText(QString::number(m_totalUavCount));
    if (ui->label_6) ui->label_6->setText(QString::number(targetTypes.size()));
}

void RoutePlanning::onPlanAllClicked()
{
    if (m_assignments.isEmpty()) {
        return;
    }

    m_planningResults.clear();
    m_pendingGroups.clear();
    m_rowPaths.clear();
    m_currentGroupIndex = 0;

    ui->tableWidget_Plan->setRowCount(0);
    ui->tableWidget_Plan->clearContents();
    ui->pushButton_Plan->setEnabled(false);
    ui->pushButton_Plan->setText("航路规划中 ...");

    // 按目标分组 UAV
    QMap<QString, QList<UavAssignment>> groups;
    for (const auto &a : m_assignments)
        groups[a.targetId].append(a);

    const double baseLon = 116.35;
    const double baseLat = 39.95;

    // 预计算所有组的规划输入
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        const QList<UavAssignment> &group = it.value();
        if (group.isEmpty()) continue;

        std::vector<GeoPoint> startPositions;
        for (int i = 0; i < group.size(); ++i) {
            double lonOff = (i % 5) * 0.008;
            double latOff = (i / 5) * 0.006;
            startPositions.push_back({
                baseLon + lonOff,
                baseLat + latOff + 0.08,
                50.0 + (i % 7) * 2.0
            });
        }

        std::vector<GeoPoint> targetArea;
        if (group.first().targetType == "AR") {
            double tLon = baseLon + 0.12;
            double tLat = baseLat - 0.06;
            targetArea.push_back({tLon, tLat, 30.0});
            targetArea.push_back({tLon + 0.05, tLat, 30.0});
            targetArea.push_back({tLon + 0.05, tLat - 0.04, 30.0});
            targetArea.push_back({tLon, tLat - 0.04, 30.0});
        } else {
            double tLon = baseLon + 0.15;
            double tLat = baseLat - 0.10;
            targetArea.push_back({tLon, tLat, 30.0});
        }

        PlanningInput input = buildPlanningInput(group, startPositions, targetArea);
        m_pendingGroups.append({input, group.first().targetName, group.size()});
    }

    m_currentGroupIndex = 0;
    processNextGroup();
}

void RoutePlanning::processNextGroup()
{
    if (m_currentGroupIndex >= m_pendingGroups.size()) {
        int totalRows = 0;
        for (const auto &r : m_planningResults)
            totalRows += r.uavCount;
        ui->pushButton_Plan->setEnabled(true);
        ui->pushButton_Plan->setText(QString("航路规划完成  (%1 架)").arg(totalRows));
        return;
    }

    const GroupPlanData &g = m_pendingGroups[m_currentGroupIndex];
    PlanningResult result = planPaths(g.input);

    if (result.success) {
        m_planningResults.append({g.uavCount, result, g.targetName});
        int n = static_cast<int>(result.approach_paths.size());
        int currentRow = ui->tableWidget_Plan->rowCount();
        ui->tableWidget_Plan->setRowCount(currentRow + n);

        // 逐行填入，每行间隔 80ms 延时
        m_currentRowInGroup = 0;
        m_groupStartRow = currentRow;
        m_currentGroupResult = result;
        m_currentGroupTargetName = g.targetName;
        m_currentGroupUavCount = g.uavCount;

        QTimer::singleShot(80, this, &RoutePlanning::appendNextRow);
    } else {
        m_currentGroupIndex++;
        QTimer::singleShot(50, this, &RoutePlanning::processNextGroup);
    }
}

void RoutePlanning::appendNextRow()
{
    if (m_currentRowInGroup >= static_cast<int>(m_currentGroupResult.approach_paths.size())) {
        m_currentGroupIndex++;
        QTimer::singleShot(100, this, &RoutePlanning::processNextGroup);
        return;
    }

    int i = m_currentRowInGroup;
    int row = m_groupStartRow + i;

    const UAVPath &approach = m_currentGroupResult.approach_paths[i];
    const UAVPath &attack = m_currentGroupResult.attack_paths[i];

    QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(row + 1));
    idItem->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget_Plan->setItem(row, 0, idItem);

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
    ui->tableWidget_Plan->setItem(row, 1, uavItem);

    QTableWidgetItem *tgtItem = new QTableWidgetItem(m_currentGroupTargetName);
    tgtItem->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget_Plan->setItem(row, 2, tgtItem);

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
    ui->tableWidget_Plan->setItem(row, 3, distItem);

    QTableWidgetItem *statusItem = new QTableWidgetItem("已生成");
    statusItem->setTextAlignment(Qt::AlignCenter);
    ui->tableWidget_Plan->setItem(row, 4, statusItem);

    m_rowPaths.append(qMakePair(approach, attack));

    QPushButton *detailBtn = new QPushButton(QString::fromUtf8("\xe6\x9f\xa5\xe7\x9c\x8b\xe8\xaf\xa6\xe6\x83\x85"));
    detailBtn->setFixedHeight(16);
    detailBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: #0f1a2e;
            border: 1px solid #1a3a6a;
            border-radius: 2px;
            color: #00b4ff;
            padding: 0px 6px;
            font-size: 10px;
        }
        QPushButton:hover {
            background-color: #0f1a3e;
            border: 1px solid #00b4ff;
            color: #00e5ff;
        }
    )"));
    ui->tableWidget_Plan->setRowHeight(row, 40);
    int capturedRow = row;
    connect(detailBtn, &QPushButton::clicked, this, [this, capturedRow]() {
        if (capturedRow < 0 || capturedRow >= m_rowPaths.size()) return;

        const UAVPath &approachPath = m_rowPaths[capturedRow].first;
        const UAVPath &attackPath = m_rowPaths[capturedRow].second;

        PathPlanning pathData;
        for (const auto &a : m_assignments) {
            if (a.uavIndex == approachPath.uav_id) {
                pathData.uavName = a.uavId;
                break;
            }
        }
        if (pathData.uavName.isEmpty())
            pathData.uavName = QString("UAV-%1").arg(approachPath.uav_id + 1, 2, 10, QChar('0'));

        QTableWidgetItem *tgtItem = ui->tableWidget_Plan->item(capturedRow, 2);
        pathData.relatedTask = tgtItem ? tgtItem->text() : QString();
        pathData.status = QString::fromUtf8("\xe5\xb7\xb2\xe7\x94\x9f\xe6\x88\x90");

        int order = 0;
        for (const auto &wp : approachPath.waypoints) {
            PathPoint pt;
            pt.pointOrder = ++order;
            pt.latitude = wp.lat;
            pt.longitude = wp.lon;
            pt.altitude = wp.alt;
            pt.pointType = QString::fromUtf8("\xe5\xb7\xa1\xe8\x88\xaa\xe8\xb7\xaf\xe5\xbe\x84");
            pathData.fightPathPoints.append(pt);
        }
        for (const auto &wp : attackPath.waypoints) {
            PathPoint pt;
            pt.pointOrder = ++order;
            pt.latitude = wp.lat;
            pt.longitude = wp.lon;
            pt.altitude = wp.alt;
            pt.pointType = QString::fromUtf8("\xe6\x90\x9c\xe7\xb4\xa2\xe8\xb7\xaf\xe5\xbe\x84");
            pathData.searchPathPoints.append(pt);
        }
        pathData.pathPointCount = order;

        PathDisplayDialog *dlg = new PathDisplayDialog(this);
        dlg->setPathData(pathData);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->exec();
    });
    ui->tableWidget_Plan->setCellWidget(row, 5, detailBtn);

    m_currentRowInGroup++;

    // 更新按钮进度
    int doneRows = m_groupStartRow + m_currentRowInGroup;
    ui->pushButton_Plan->setText(QString("航路规划中 ... (%1/%2)")
                                     .arg(doneRows)
                                     .arg(m_planningResults.last().uavCount + m_groupStartRow));

    QTimer::singleShot(80, this, &RoutePlanning::appendNextRow);
}

PlanningInput RoutePlanning::buildPlanningInput(const QList<UavAssignment> &group,
                                                 const std::vector<GeoPoint> &startPositions,
                                                 const std::vector<GeoPoint> &targetArea)
{
    PlanningInput input;

    input.start_positions = startPositions;
    input.target_area = targetArea;
    input.uav_count = static_cast<int>(group.size());

    input.cruise_alt = parseValue(ui->label_ClimbHeightValue->text());
    if (input.cruise_alt < 100.0) input.cruise_alt = 800.0;

    input.cruise_speed_mps = parseSpeed(ui->label_CruiseSpeedValue->text());
    if (input.cruise_speed_mps < 1.0) input.cruise_speed_mps = 60.0;

    {
        double sh = parseValue(ui->label_AreaSearchHeightValue->text());
        input.search_radius = (sh > 10.0) ? sh * 4.0 : 2000.0;
    }

    input.waypoint_spacing_m = 400.0;

    QString searchForm = ui->comboBox_SearchForm->currentText();
    if (searchForm.contains(QString::fromUtf8("8")))
        input.search_pattern = SearchPattern::FIGURE8;
    else
        input.search_pattern = SearchPattern::SPIRAL;

    QString cruiseMode = ui->comboBox_CruiseMode->currentText();
    if (cruiseMode.contains("中空")) {
        input.cruise_alt = 3000.0;
    } else if (cruiseMode.contains("地形跟随")) {
        input.cruise_alt = qMin(input.cruise_alt, 200.0);
    }

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

double RoutePlanning::parseValue(const QString &text)
{
    QString digits;
    for (const QChar &ch : text) {
        if (ch.isDigit() || ch == '.')
            digits.append(ch);
    }
    return digits.toDouble();
}

double RoutePlanning::parseSpeed(const QString &text)
{
    double kmh = parseValue(text);
    return kmh / 3.6;
}

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

    // widget_5 按钮容器：添加琥珀色边框，与 HTML 中 rp-action-bar 风格一致
    ui->widget_5->setStyleSheet(QString(R"(
        QWidget#widget_5 {
            background-color: %1;
            border: 1px solid #8a6010;
            border-left: 3px solid #ffb627;
            border-radius: 4px;
        }
    )").arg(panelBg));

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

    // pushButton_Plan 主按钮：琥珀色风格，与 HTML 中 btn-plan 一致
    ui->pushButton_Plan->setStyleSheet(QString(R"(
        QPushButton {
            background-color: #ffb627;
            border: 1px solid #ffb627;
            border-radius: 4px;
            color: #0a0e0f;
            padding: 6px 16px;
            font-size: 13px;
            font-weight: bold;
            min-height: 36px;
            letter-spacing: 2px;
        }
        QPushButton:hover {
            background-color: #ffc547;
            border: 1px solid #ffb627;
            color: #0a0e0f;
        }
        QPushButton:pressed {
            background-color: #e6a420;
        }
        QPushButton:disabled {
            background-color: #8a6010;
            border: 1px solid #8a6010;
            color: #5a6e75;
        }
    )"));

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

    ui->tableWidget_Plan->setStyleSheet(tableStyle);

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

    // widget_5 内统计标签样式
    QString statLabelStyle = QString(R"(
        QLabel {
            color: %1;
            font-size: 12px;
            font-weight: bold;
            background: transparent;
            border: none;
            padding: 1px 4px;
        }
    )").arg(accentCyan);
    if (ui->label) ui->label->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-size: 11px;
            font-weight: normal;
            background: transparent;
            border: none;
            padding: 1px 4px;
        }
    )").arg(textSecondary));
    if (ui->label_2) ui->label_2->setStyleSheet(statLabelStyle);
    if (ui->label_3) ui->label_3->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-size: 11px;
            font-weight: normal;
            background: transparent;
            border: none;
            padding: 1px 4px;
        }
    )").arg(textSecondary));
    if (ui->label_4) ui->label_4->setStyleSheet(statLabelStyle);
    if (ui->label_5) ui->label_5->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-size: 11px;
            font-weight: normal;
            background: transparent;
            border: none;
            padding: 1px 4px;
        }
    )").arg(textSecondary));
    if (ui->label_6) ui->label_6->setStyleSheet(statLabelStyle);

    // 初始化表格列
    QStringList headers = {"编号", "无人机", "目标", "总航程", "状态", "操作"};
    ui->tableWidget_Plan->setColumnCount(6);
    ui->tableWidget_Plan->setHorizontalHeaderLabels(headers);
    ui->tableWidget_Plan->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableWidget_Plan->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget_Plan->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableWidget_Plan->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->tableWidget_Plan->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->tableWidget_Plan->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    ui->tableWidget_Plan->horizontalHeader()->resizeSection(5, 100);
    ui->tableWidget_Plan->verticalHeader()->setVisible(false);
    ui->tableWidget_Plan->verticalHeader()->setDefaultSectionSize(24);
    ui->tableWidget_Plan->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget_Plan->setSelectionBehavior(QTableWidget::SelectRows);
    ui->tableWidget_Plan->setAlternatingRowColors(true);
    ui->tableWidget_Plan->setEditTriggers(QTableWidget::NoEditTriggers);
}