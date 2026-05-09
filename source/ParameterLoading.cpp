//
// Created by Administrator on 2026/5/7.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ParameterLoading.h" resolved

#include "ParameterLoading.h"
#include "ui_ParameterLoading.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QAbstractSpinBox>


ParameterLoading::ParameterLoading(QWidget *parent) :
        QWidget(parent), ui(new Ui::ParameterLoading) {
    ui->setupUi(this);

    setupDroneTable();
    applyTechStyle();
    connectSignals();
    updateStatusBar();
}

ParameterLoading::~ParameterLoading() {
    delete ui;
}

void ParameterLoading::setAssignmentData(const QList<UavAssignment> &assignments,
                                          const QList<UavResource> &resourcePool)
{
    m_assignments = assignments;
    m_resourcePool = resourcePool;
    populateSampleData();
    m_selectedCount = 0;
    ui->selectedCount->setText("0");
    ui->selectAllCheckbox->blockSignals(true);
    ui->selectAllCheckbox->setChecked(false);
    ui->selectAllCheckbox->blockSignals(false);
    updateStatusBar();
}

void ParameterLoading::applyTechStyle()
{
    const QString baseBg = "#0a0e1a";
    const QString panelBg = "#0d1326";
    const QString borderColor = "#1a3a6a";
    const QString accentBlue = "#00b4ff";
    const QString accentCyan = "#00e5ff";
    const QString textPrimary = "#e0e8f0";
    const QString textSecondary = "#7a8ba8";
    const QString inputBg = "#0f1a2e";
    const QString groupBoxBg = "#0b1124";
    const QString hoverBorder = "#00b4ff";

    setStyleSheet(QString(R"(
        ParameterLoading {
            background-color: %1;
        }
        QWidget {
            color: %2;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
        }
    )").arg(baseBg).arg(textPrimary));

    ui->widget_back->setStyleSheet(QString(R"(
        QWidget#widget_back {
            background-color: %1;
        }
    )").arg(baseBg));

    QString cardStyle = QString(R"(
        QWidget#cardRadar, QWidget#cardSeeker {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
        }
    )").arg(panelBg).arg(borderColor);
    ui->cardRadar->setStyleSheet(cardStyle);
    ui->cardSeeker->setStyleSheet(cardStyle);

    QString headerStyle = QString(R"(
        QWidget#radarHeader, QWidget#seekerHeader, QWidget#tableHeader {
            background-color: %1;
            border-bottom: 1px solid %2;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
    )").arg(groupBoxBg).arg(borderColor);
    ui->radarHeader->setStyleSheet(headerStyle);
    ui->seekerHeader->setStyleSheet(headerStyle);

    QString titleLabelStyle = QString(R"(
        QLabel#lblRadarTitle, QLabel#lblSeekerTitle, QLabel#lblTableTitle {
            color: %1;
            font-size: 13px;
            font-weight: bold;
            background: transparent;
            border: none;
        }
    )").arg(textPrimary);
    ui->lblRadarTitle->setStyleSheet(titleLabelStyle);
    ui->lblSeekerTitle->setStyleSheet(titleLabelStyle);

    QString badgeStyle = QString(R"(
        QLabel#lblRadarBadge, QLabel#lblSeekerBadge {
            color: %1;
            font-size: 9px;
            font-weight: bold;
            background-color: %2;
            border: 1px solid %1;
            border-radius: 3px;
            padding: 1px 8px;
        }
    )").arg(accentBlue).arg(inputBg);
    ui->lblRadarBadge->setStyleSheet(badgeStyle);
    ui->lblSeekerBadge->setStyleSheet(badgeStyle);

    QString fieldLabelStyle = QString(R"(
        QLabel {
            color: %1;
            font-size: 11px;
            font-weight: normal;
            background: transparent;
            border: none;
            padding: 0px;
        }
    )").arg(textSecondary);
    for (QLabel* lbl : findChildren<QLabel*>()) {
        QString name = lbl->objectName();
        if (name == "lblRadarTitle" || name == "lblSeekerTitle" ||
            name == "lblTableTitle" || name == "lblRadarBadge" ||
            name == "lblSeekerBadge" || name == "lblStatusLive" ||
            name == "lblStatusInfo" || name == "lblStatusHint" ||
            name == "lblBurstHint" || name == "lblSelectedPrefix" ||
            name == "lblSelectedSuffix" || name == "selectedCount") {
            continue;
        }
        lbl->setStyleSheet(fieldLabelStyle);
    }

    QString burstHintStyle = QString(R"(
        QLabel#lblBurstHint {
            color: %1;
            font-size: 10px;
            font-weight: normal;
            background: transparent;
            border: none;
            padding: 0px;
        }
    )").arg(accentCyan);
    ui->lblBurstHint->setStyleSheet(burstHintStyle);

    ui->lblBurstHeight->setStyleSheet(QString(R"(
        QLabel#lblBurstHeight {
            color: %1;
            font-size: 11px;
            font-weight: bold;
            background: transparent;
            border: none;
            padding: 0px;
        }
    )").arg(accentCyan));

    QString spinBoxStyle = QString(R"(
        QAbstractSpinBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 2px 8px;
            min-height: 26px;
            font-size: 12px;
        }
        QAbstractSpinBox:focus {
            border: 1px solid %4;
            background-color: %5;
        }
        QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
            width: 16px;
            border-left: 1px solid %2;
        }
    )").arg(inputBg).arg(borderColor).arg(textPrimary)
       .arg(hoverBorder).arg("#0a1428");
    for (QAbstractSpinBox* sb : findChildren<QAbstractSpinBox*>()) {
        sb->setStyleSheet(spinBoxStyle);
    }

    QString comboBoxStyle = QString(R"(
        QComboBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 2px 8px;
            min-height: 26px;
            font-size: 12px;
        }
        QComboBox:focus {
            border: 1px solid %4;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left: 1px solid %2;
        }
        QComboBox::down-arrow {
            width: 10px;
            height: 10px;
        }
        QComboBox QAbstractItemView {
            background-color: %1;
            border: 1px solid %2;
            color: %3;
            selection-background-color: %6;
            selection-color: %7;
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
            border-top: none;
            gridline-color: %3;
            color: %4;
            font-size: 11px;
            selection-background-color: %5;
            selection-color: %6;
        }
        QTableWidget::item {
            padding: 4px 6px;
        }
        QTableWidget::item:alternate {
            background-color: %7;
        }
        QHeaderView::section {
            background-color: %8;
            border: 1px solid %2;
            color: %9;
            font-weight: bold;
            font-size: 11px;
            padding: 4px 6px;
        }
        QScrollBar:vertical {
            background: #0a0e1a;
            width: 6px;
            margin: 0;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical {
            background: #1a3a6a;
            min-height: 30px;
            border-radius: 3px;
        }
        QScrollBar::handle:vertical:hover {
            background: #00b4ff;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background: #0a0e1a;
            height: 6px;
            margin: 0;
            border-radius: 3px;
        }
        QScrollBar::handle:horizontal {
            background: #1a3a6a;
            min-width: 30px;
            border-radius: 3px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #00b4ff;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
    )").arg(panelBg).arg(borderColor).arg("#1a2a4a")
       .arg(textPrimary).arg("#0d2466").arg("#ffffff")
       .arg("#0a1428").arg("#0a1628").arg(accentBlue);
    ui->droneTable->setStyleSheet(tableStyle);

    QString selectAllStyle = QString(R"(
        QWidget#selectAllRow {
            background-color: %1;
            border-left: 1px solid %2;
            border-right: 1px solid %2;
        }
    )").arg(groupBoxBg).arg(borderColor);
    ui->selectAllRow->setStyleSheet(selectAllStyle);

    QString selectAllCheckStyle = QString(R"(
        QCheckBox {
            color: %1;
            font-size: 11px;
            background: transparent;
        }
        QCheckBox::indicator {
            width: 14px;
            height: 14px;
        }
    )").arg(textPrimary);
    ui->selectAllCheckbox->setStyleSheet(selectAllCheckStyle);

    for (QLabel* lbl : {ui->lblSelectedPrefix, ui->lblSelectedSuffix}) {
        lbl->setStyleSheet(QString(R"(
            QLabel {
                color: %1;
                font-size: 11px;
                background: transparent;
                border: none;
            }
        )").arg(textSecondary));
    }
    ui->selectedCount->setStyleSheet(QString(R"(
        QLabel#selectedCount {
            color: %1;
            font-size: 12px;
            font-weight: bold;
            background: transparent;
            border: none;
        }
    )").arg(accentCyan));

    QString actionBarStyle = QString(R"(
        QWidget#actionBar {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
        }
    )").arg(panelBg).arg(borderColor);
    ui->actionBar->setStyleSheet(actionBarStyle);

    QString statusBarStyle = QString(R"(
        QWidget#statusBar {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
        }
    )").arg(groupBoxBg).arg(borderColor);


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
        }
    )").arg(inputBg).arg(borderColor).arg(textPrimary)
       .arg("#0f1a3e").arg(hoverBorder).arg(accentCyan)
       .arg("#081020");

    ui->verifyBtn->setStyleSheet(buttonTechStyle);
    ui->defaultBtn->setStyleSheet(buttonTechStyle);


    QString batchBtnStyle = QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %1;
            border-radius: 4px;
            color: %2;
            padding: 4px 16px;
            font-size: 12px;
            font-weight: bold;
            min-height: 30px;
        }
        QPushButton:hover {
            background-color: %3;
            border-color: %3;
            color: %2;
        }
        QPushButton:pressed {
            background-color: %4;
        }
    )").arg("#1a5a8a").arg("#ffffff").arg("#0078d4").arg("#0f3a6a");
    ui->batchUploadBtn->setStyleSheet(batchBtnStyle);

    ui->tableSectionWidget->setStyleSheet(QString(R"(
        QWidget#tableSectionWidget {
            background-color: transparent;
        }
    )"));



    QString burstWrapperStyle = QString(R"(
        QWidget#rowBurstWrapper {
            background-color: %1;
            border: 1px solid %2;
            border-left: 3px solid %3;
            border-radius: 4px;
        }
    )").arg("#0a1428").arg(borderColor).arg(accentCyan);

    ui->rowDivider->setStyleSheet(QString(R"(
        QWidget#rowDivider {
            background-color: %1;
            border: none;
        }
    )").arg(borderColor));
}

void ParameterLoading::setupDroneTable()
{
    QStringList headers = {"选择", "无人机编号", "机型/频段", "装订进度",
                           "导引头/炸高状态", "攻击目标", "预置炸高(m)",
                           "数据链质量", "就绪"};
    ui->droneTable->setColumnCount(9);
    ui->droneTable->setHorizontalHeaderLabels(headers);

    QHeaderView *horizontal = ui->droneTable->horizontalHeader();
    horizontal->setSectionResizeMode(0, QHeaderView::Fixed);
    horizontal->resizeSection(0, 50);
    horizontal->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    horizontal->setSectionResizeMode(2, QHeaderView::Stretch);
    horizontal->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    horizontal->setSectionResizeMode(4, QHeaderView::Stretch);
    horizontal->setSectionResizeMode(5, QHeaderView::Stretch);
    horizontal->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    horizontal->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    horizontal->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    horizontal->setDefaultAlignment(Qt::AlignCenter);

    ui->droneTable->verticalHeader()->setVisible(false);
    ui->droneTable->verticalHeader()->setDefaultSectionSize(28);
    ui->droneTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->droneTable->setSelectionBehavior(QTableWidget::SelectRows);
    ui->droneTable->setAlternatingRowColors(true);
    ui->droneTable->setEditTriggers(QTableWidget::NoEditTriggers);

    populateSampleData();
}

void ParameterLoading::populateSampleData()
{
    m_droneCheckBoxes.clear();
    ui->droneTable->setRowCount(0);

    if (m_assignments.isEmpty())
    {
        updateStatusBar();
        return;
    }

    for (int i = 0; i < m_assignments.size(); ++i)
    {
        const UavAssignment &assignment = m_assignments[i];

        int row = ui->droneTable->rowCount();
        ui->droneTable->insertRow(row);
        ui->droneTable->setRowHeight(row, 28);

        QCheckBox *cb = new QCheckBox(ui->droneTable);
        cb->setStyleSheet(QString(
            "QCheckBox { background: transparent; }"
            "QCheckBox::indicator { width: 14px; height: 14px; }"
        ));
        QWidget *cbContainer = new QWidget(ui->droneTable);
        QHBoxLayout *cbLayout = new QHBoxLayout(cbContainer);
        cbLayout->setContentsMargins(0, 0, 0, 0);
        cbLayout->setAlignment(Qt::AlignCenter);
        cbLayout->addWidget(cb);
        ui->droneTable->setCellWidget(row, 0, cbContainer);
        connect(cb, &QCheckBox::toggled, this, &ParameterLoading::onDroneCheckChanged);
        m_droneCheckBoxes.append(cb);

        auto setItem = [&](int col, const QString &text, const QString &color = QString()) {
            QTableWidgetItem *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            if (!color.isEmpty()) {
                item->setForeground(QColor(color));
            }
            ui->droneTable->setItem(row, col, item);
        };

        QString typeName;
        for (const UavResource &res : m_resourcePool)
        {
            if (res.uavIndex == assignment.uavIndex)
            {
                typeName = res.typeName;
                break;
            }
        }
        if (typeName.isEmpty())
            typeName = (assignment.targetType == "AR") ? "AR-1 / H/I" : "PT-1 / I";

        setItem(1, assignment.uavId);
        setItem(2, typeName);
        setItem(3, "待上传(0%)", "#ffb86b");
        setItem(4, "待配置");
        setItem(5, assignment.targetName);
        setItem(6, "30");
        setItem(7, "-70dBm");
        setItem(8, "否", "#ff7a7a");
    }
}

void ParameterLoading::connectSignals()
{
    connect(ui->selectAllCheckbox, &QCheckBox::stateChanged,
            this, &ParameterLoading::onSelectAllChanged);

    connect(ui->verifyBtn, &QPushButton::clicked,
            this, &ParameterLoading::onVerifyClicked);
    connect(ui->defaultBtn, &QPushButton::clicked,
            this, &ParameterLoading::onDefaultClicked);
    connect(ui->batchUploadBtn, &QPushButton::clicked,
            this, &ParameterLoading::onBatchUploadClicked);
}

void ParameterLoading::onSelectAllChanged(int state)
{
    bool checked = (state == Qt::Checked);
    for (QCheckBox *cb : m_droneCheckBoxes) {
        cb->blockSignals(true);
        cb->setChecked(checked);
        cb->blockSignals(false);
    }
    m_selectedCount = checked ? m_droneCheckBoxes.size() : 0;
    ui->selectedCount->setText(QString::number(m_selectedCount));
}

void ParameterLoading::onDroneCheckChanged()
{
    int count = 0;
    for (QCheckBox *cb : m_droneCheckBoxes) {
        if (cb->isChecked()) ++count;
    }
    m_selectedCount = count;
    ui->selectedCount->setText(QString::number(count));

    bool allChecked = (count == m_droneCheckBoxes.size());
    ui->selectAllCheckbox->blockSignals(true);
    ui->selectAllCheckbox->setChecked(allChecked);
    ui->selectAllCheckbox->blockSignals(false);
}

void ParameterLoading::onVerifyClicked()
{
    QMessageBox::information(this, "参数校验",
                             "参数校验通过，所有辐射源参数格式正确。");
}

void ParameterLoading::onDefaultClicked()
{
    restoreDefaults();
}

void ParameterLoading::restoreDefaults()
{
    ui->rfFreqMin->setValue(8.0);
    ui->rfFreqMax->setValue(10.0);
    ui->pulseWidthMin->setValue(0.5);
    ui->pulseWidthMax->setValue(2.5);
    ui->prfMin->setValue(1000);
    ui->prfMax->setValue(5000);
    ui->threatLevel->setCurrentIndex(0);


    ui->searchMode->setCurrentIndex(0);
    ui->sectorStart->setValue(-30);
    ui->sectorEnd->setValue(30);
    ui->elevMin->setValue(-15);
    ui->elevMax->setValue(25);

    ui->burstHeight->setValue(35);
    ui->fuseTypeSimple->setCurrentIndex(0);

    updateStatusBar();
}

void ParameterLoading::onBatchUploadClicked()
{
    if (m_selectedCount == 0) {
        QMessageBox::warning(this, "装订提示",
                             "请先勾选需要装订的无人机。");
        return;
    }

    const QString burstHeight = QString::number(ui->burstHeight->value());
    const QString searchMode = ui->searchMode->currentText();

    QString seekerStatus;
    if (searchMode.contains("归向")) {
        seekerStatus = QString("归向锁定 · %1m").arg(burstHeight);
    } else if (searchMode.contains("螺旋")) {
        seekerStatus = QString("螺旋扫描 · %1m").arg(burstHeight);
    } else if (searchMode.contains("扇形")) {
        seekerStatus = QString("扇形扫描 · %1m").arg(burstHeight);
    } else {
        seekerStatus = QString("已配置 · %1m").arg(burstHeight);
    }

    for (int row = 0; row < m_droneCheckBoxes.size(); ++row) {
        if (!m_droneCheckBoxes[row]->isChecked())
            continue;

        auto updateCell = [&](int col, const QString &text, const QString &color = QString()) {
            QTableWidgetItem *item = ui->droneTable->item(row, col);
            if (!item) {
                item = new QTableWidgetItem();
                ui->droneTable->setItem(row, col, item);
            }
            item->setText(text);
            item->setTextAlignment(Qt::AlignCenter);
            if (!color.isEmpty())
                item->setForeground(QColor(color));
        };

        updateCell(3, "已上传(100%)", "#68d391");
        updateCell(4, seekerStatus);
        updateCell(6, burstHeight);
        updateCell(8, "是", "#68d391");
    }

    QMessageBox::information(this, "批量装订",
                             QString("已将融合参数装订至 %1 架无人机。").arg(m_selectedCount));
}

void ParameterLoading::onRefreshTableClicked()
{
    populateSampleData();
    m_selectedCount = 0;
    ui->selectedCount->setText("0");
    ui->selectAllCheckbox->blockSignals(true);
    ui->selectAllCheckbox->setChecked(false);
    ui->selectAllCheckbox->blockSignals(false);
}

void ParameterLoading::updateStatusBar()
{
    QString freq = QString("%1~%2 GHz").arg(
        QString::number(ui->rfFreqMin->value(), 'f', 1),
        QString::number(ui->rfFreqMax->value(), 'f', 1));
    QString mode = ui->searchMode->currentText();
    QString burst = QString::number(ui->burstHeight->value());

}
