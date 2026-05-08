//
// Created by Administrator on 2026/5/6.
//
// 兵力需求计算面板 - 实现文件

// You may need to build the project (run Qt uic code generator) to get "ui_ForceRequirementPanel.h" resolved

#include "ForceRequirementPanel.h"
#include "ui_ForceRequirementPanel.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QDebug>
#include <QtMath>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>

// 构造函数：初始化UI、设置雷达目标数据、应用样式、连接信号槽
ForceRequirementPanel::ForceRequirementPanel(QWidget *parent) : QFrame(parent), ui(new Ui::ForceRequirementPanel)
{
    ui->setupUi(this);

    // 获取 groupBox_Target 已有网格布局
    m_targetGrid = qobject_cast<QGridLayout*>(ui->groupBox_Target->layout());
    if (!m_targetGrid) {
        qDebug() << "[ERROR] gridLayout not found on groupBox_Target!";
    } else {
        m_targetGrid->setSpacing(6);
        m_targetGrid->setContentsMargins(6, 6, 6, 6);
    }

    // 初始显示点目标模型页面
    ui->stackedWidget->setCurrentIndex(0);
    m_currentTargetId = "";
    m_currentTargetType = "";
    m_currentDamageLevel = 0.9; // 默认高毁伤要求

    // ⚠️ 先应用全局样式，再创建 pill（确保 pill 的 inline 样式不会被父级 QSS 覆盖）
    applyTechStyle();

    // 填充默认计算目标
    setupDefaultTargets();
    if (m_targetPills.size() > 0) selectTarget(0);

    setupConnections(); // 初始化信号槽连接
    setupPtInputConnections(); // 初始化点目标输入参数信号槽
    setupArInputConnections(); // 初始化区域目标输入参数信号槽
    updatePtCalculation();     // 初始计算点目标结果
    updateArCalculation();     // 初始计算区域目标结果
}

// 析构函数
ForceRequirementPanel::~ForceRequirementPanel()
{
    delete ui;
}

// 初始化信号槽连接：绑定计算过程和兵力汇总按钮的点击事件
void ForceRequirementPanel::setupConnections()
{
    connect(ui->btn_CalculateProcess, &QPushButton::clicked, this, &ForceRequirementPanel::onCalculateProcessClicked);
    connect(ui->btn_TotalForceSummary, &QPushButton::clicked, this, &ForceRequirementPanel::onTotalForceSummaryClicked);
}

// 初始化点目标输入参数信号槽：毁伤要求按钮、单弹毁伤概率、备份架数变化时重新计算
void ForceRequirementPanel::setupPtInputConnections()
{
    connect(ui->btn_DamageLevel_Low, &QPushButton::clicked, this, [this]() {
        m_currentDamageLevel = 0.7;
        updatePtCalculation();
    });
    connect(ui->btn_DamageLevel_Mid, &QPushButton::clicked, this, [this]() {
        m_currentDamageLevel = 0.8;
        updatePtCalculation();
    });
    connect(ui->btn_DamageLevel_High, &QPushButton::clicked, this, [this]() {
        m_currentDamageLevel = 0.9;
        updatePtCalculation();
    });
    connect(ui->spinBox_SingleShotProb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        updatePtCalculation();
    });
    connect(ui->spinBox_BackupCount, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        updatePtCalculation();
    });
}

// 初始化区域目标输入参数信号槽：区域面积、估计目标数、每目标弹数、备份架数变化时重新计算
void ForceRequirementPanel::setupArInputConnections()
{
    connect(ui->spinBox_Area, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        updateArCalculation();
    });
    connect(ui->spinBox_EstTargetCount, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        updateArCalculation();
    });
    connect(ui->spinBox_ShotsPerTarget, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        updateArCalculation();
    });
    connect(ui->spinBox_AreaBackupCount, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        updateArCalculation();
    });
}

// 更新点目标计算结果：根据毁伤要求P̄和单弹毁伤概率Pk计算所需弹数n = ⌈log(1-P̄)/log(1-Pk)⌉，再加备份架数
void ForceRequirementPanel::updatePtCalculation()
{
    double P_bar = m_currentDamageLevel;
    double Pk = ui->spinBox_SingleShotProb->value();
    int backup = ui->spinBox_BackupCount->value();

    double logNumerator = qLn(1.0 - P_bar);
    double logDenominator = qLn(1.0 - Pk);
    double quotient = logNumerator / logDenominator;
    int n = static_cast<int>(qCeil(quotient));
    int total = n + backup;

    ui->btn_cal1->setText(QString("P̄=%1").arg(P_bar, 0, 'f', 2));
    ui->btn_cal2->setText(QString("log(1-%1)=%2").arg(P_bar, 0, 'f', 2).arg(logNumerator, 0, 'f', 3));
    ui->btn_cal3->setText(QString("Pk=%1").arg(Pk, 0, 'f', 2));
    ui->btn_cal4->setText(QString("log(1-%1)=%2").arg(Pk, 0, 'f', 2).arg(logDenominator, 0, 'f', 3));

    ui->btn_cal5->setText(QString::number(quotient, 'f', 3));
    ui->btn_cal6->setText(QString("%1弹").arg(n));

    ui->btn_cal7->setText(QString("n = %1").arg(n));
    ui->btn_cal8->setText(QString("备份= %1").arg(backup));
    ui->btn_cal9->setText(QString("%1架").arg(total));

    ui->label_CalculationResult->setText(QString("%1架").arg(total));
    ui->label_ResultDescription->setText(
        QString("按照%1弹齐射+%2架备份编组，预计达成毁伤率>=%3")
            .arg(n).arg(backup).arg(P_bar, 0, 'f', 2));

    PtCalcData data;
    data.damageLevel = P_bar;
    data.Pk = Pk;
    data.n = n;
    data.backup = backup;
    data.total = total;
    m_ptResults[m_currentTargetId] = data;
}

// 更新区域目标计算结果：N搜 = ⌈面积/25⌉，N打 = ⌈估计目标数×每目标弹数⌉，总 = MAX(N搜,N打) + 备份
void ForceRequirementPanel::updateArCalculation()
{
    const double coveragePerUav = 25.0; // 单机覆盖面积 KM²
    int area = ui->spinBox_Area->value();
    double estTargets = ui->spinBox_EstTargetCount->value();
    int shotsPerTarget = ui->spinBox_ShotsPerTarget->value();
    int backup = ui->spinBox_AreaBackupCount->value();

    int N_search = static_cast<int>(qCeil(area / coveragePerUav));
    int N_strike = static_cast<int>(qCeil(estTargets * shotsPerTarget));
    int N_max = qMax(N_search, N_strike);
    int total = N_max + backup;

    ui->btn_cal10->setText(QString("面积 = %1 KM²").arg(area));
    ui->btn_cal12->setText(QString("%1架").arg(N_search));
    ui->btn_cal13->setText(QString("M = %1 部").arg(estTargets, 0, 'f', 1));
    ui->btn_cal14->setText(QString("n = %1弹/部").arg(shotsPerTarget));
    ui->btn_cal15->setText(QString("%1架").arg(N_strike));
    ui->btn_cal16->setText(QString("MAX(%1,%2) = %3").arg(N_search).arg(N_strike).arg(N_max));
    ui->btn_cal17->setText(QString("备份 = %1").arg(backup));
    ui->btn_cal18->setText(QString("%1架").arg(total));

    ui->label_TotalResult->setText(QString("%1架").arg(total));

    ArCalcData data;
    data.area = area;
    data.estTargets = estTargets;
    data.shotsPerTarget = shotsPerTarget;
    data.backup = backup;
    data.N_search = N_search;
    data.N_strike = N_strike;
    data.total = total;
    m_arResults[m_currentTargetId] = data;
}

void ForceRequirementPanel::applyTechStyle()
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
    const QString dangerRed = "#ff3355";
    const QString successGreen = "#00e676";

    setStyleSheet(QString(R"(
        ForceRequirementPanel {
            background-color: %1;
            border: 2px solid %2;
            border-radius: 6px;
        }
        QWidget {
            color: %3;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
        }
    )").arg(baseBg).arg(borderColor).arg(textPrimary));

    ui->label_Title->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-size: 18px;
            font-weight: bold;
            padding: 6px 0px;
            background: transparent;
            border: none;
        }
    )").arg(accentBlue));

    ui->label_PointModel->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-size: 13px;
            font-weight: bold;
            padding: 4px 0px;
            background: transparent;
            border: none;
        }
    )").arg(accentCyan));

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
        // 跳过目标选择 pill 按钮（带 targetPill 属性标记）
        if (btn->property("targetPill").toBool()) continue;
        btn->setStyleSheet(buttonTechStyle);
    }

    ui->btn_CalculateProcess->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 6px 16px;
            font-size: 13px;
            font-weight: bold;
            min-height: 32px;
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

    ui->btn_TotalForceSummary->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 6px 16px;
            font-size: 13px;
            font-weight: bold;
            min-height: 32px;
        }
        QPushButton:hover {
            background-color: %4;
            border: 1px solid %5;
            color: %6;
        }
        QPushButton:pressed {
            background-color: %7;
        }
    )").arg("#0a1628").arg(successGreen).arg(successGreen)
       .arg("#0f1a3e").arg(successGreen).arg("#ffffff")
       .arg("#060e1a"));

    QString lineEditStyle = QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 2px 8px;
            font-size: 12px;
            min-height: 26px;
        }
        QLineEdit:focus {
            border: 1px solid %4;
            background-color: %5;
        }
        QLineEdit:disabled {
            background-color: %6;
            color: %7;
        }
    )").arg(inputBg).arg(inputBorder).arg(textPrimary)
       .arg(hoverBorder).arg("#0a1428")
       .arg("#060a14").arg(textSecondary);

    for (QLineEdit* le : findChildren<QLineEdit*>()) {
        le->setStyleSheet(lineEditStyle);
    }

    QString spinBoxStyle = QString(R"(
        QSpinBox, QDoubleSpinBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 2px 4px;
            font-size: 13px;
            font-weight: bold;
            min-height: 26px;
        }
        QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid %4;
        }
        QSpinBox::up-button, QDoubleSpinBox::up-button {
            background-color: %5;
            border: 1px solid %2;
            border-radius: 2px;
            width: 20px;
            subcontrol-origin: border;
            subcontrol-position: top right;
        }
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            background-color: %5;
            border: 1px solid %2;
            border-radius: 2px;
            width: 20px;
            subcontrol-origin: border;
            subcontrol-position: bottom right;
        }
        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
            image: none;
            width: 0px;
            height: 0px;
        }
        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
            image: none;
            width: 0px;
            height: 0px;
        }
    )").arg(inputBg).arg(inputBorder).arg(accentCyan)
       .arg(hoverBorder).arg("#0a1428");

    for (QSpinBox* sb : findChildren<QSpinBox*>()) {
        sb->setStyleSheet(spinBoxStyle);
        sb->setButtonSymbols(QAbstractSpinBox::PlusMinus);
    }
    for (QDoubleSpinBox* dsb : findChildren<QDoubleSpinBox*>()) {
        dsb->setStyleSheet(spinBoxStyle);
        dsb->setButtonSymbols(QAbstractSpinBox::PlusMinus);
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
        "label_DamageLevel", "label_DamageLevelEn", "label_DestroyProb",
        "label_SingleShotProb", "label_SingleShotProbEn", "label_WeaponManual",
        "label_BackupCount", "label_BackupCountEn", "label_DefaultBackup",
        "label_Area", "label_AreaEn", "label_AreaModel",
        "label_EstTargetCount", "label_EstTargetCountEn", "label_WeaponManual2",
        "label_IntelligenceNote", "label_ShotsPerTarget", "label_ShotsPerTargetEn",
        "label_ShotsPerUnit", "label_FormulaNote", "label_AreaBackupCount",
        "label_AreaBackupCountEn", "label_AreaSuggestion", "label_AreaUncertaintyNote",
        "label_FinalSuggestion", "label_TotalResult", "label_AttackStrategy",
        "label_AreaResultDisplay", "label_DivisionSign", "label_Equal2",
        "label_AreaResult2", "label_DivisionSign2", "label_Equal3",
        "label_AreaFinalResult", "label_DivisionSign3", "label_Equal4",
        "label_Arrow1", "label_Arrow2", "label_Arrow3",
        "label_Quotient", "label_RoundUp", "label_ResultDisplay",
        "label_PlusSign", "label_Equal", "label_SuggestedLaunch",
        "label_CalculationResult", "label_ResultDescription",
        "label_AreaUnit", "label_AreaSuggestion"
    };

    for (const QString& name : labelNames) {
        QLabel* label = findChild<QLabel*>(name);
        if (!label) continue;

        QString color = textSecondary;
        int fontSize = 12;
        bool isBold = false;

        if (name == "label_DamageLevel" || name == "label_Area" ||
            name == "label_EstTargetCount" || name == "label_ShotsPerTarget" ||
            name == "label_AreaBackupCount" || name == "label_BackupCount") {
            color = textPrimary;
            fontSize = 13;
            isBold = true;
        } else if (name == "label_IntelligenceNote" || name == "label_FormulaNote" ||
                   name == "label_AreaUncertaintyNote" || name == "label_DefaultBackup") {
            color = "#ffaa44";
            fontSize = 11;
        } else if (name == "label_TotalResult" || name == "label_FinalSuggestion") {
            color = accentCyan;
            fontSize = 15;
            isBold = true;
        } else if (name == "label_AttackStrategy") {
            color = textSecondary;
            fontSize = 11;
        } else if (name == "label_SuggestedLaunch") {
            color = textPrimary;
            fontSize = 13;
        } else if (name == "label_CalculationResult") {
            color = accentCyan;
            fontSize = 14;
            isBold = true;
        } else if (name == "label_ResultDescription") {
            color = textSecondary;
            fontSize = 11;
        } else if (name == "label_AreaResultDisplay" || name == "label_AreaResult2" ||
                   name == "label_AreaFinalResult") {
            color = accentCyan;
            fontSize = 12;
        } else if (name == "label_DivisionSign" || name == "label_DivisionSign2" ||
                   name == "label_DivisionSign3") {
            color = textSecondary;
            fontSize = 14;
            isBold = true;
        } else if (name == "label_Equal" || name == "label_Equal2" ||
                   name == "label_Equal3" || name == "label_Equal4") {
            color = accentBlue;
            fontSize = 14;
            isBold = true;
        } else if (name == "label_Arrow1" || name == "label_Arrow2" || name == "label_Arrow3") {
            color = accentBlue;
            fontSize = 16;
            isBold = true;
        } else if (name == "label_Quotient" || name == "label_RoundUp") {
            color = accentCyan;
            fontSize = 12;
            isBold = true;
        } else if (name == "label_ResultDisplay") {
            color = accentCyan;
            fontSize = 12;
        } else if (name == "label_PlusSign") {
            color = accentBlue;
            fontSize = 14;
            isBold = true;
        } else if (name == "label_AreaUnit") {
            color = textSecondary;
            fontSize = 11;
        } else if (name == "label_AreaModel") {
            color = accentCyan;
            fontSize = 12;
            isBold = true;
        } else if (name == "label_AreaSuggestion") {
            color = "#ffaa44";
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

    ui->stackedWidget->setStyleSheet(QString(R"(
        QStackedWidget {
            background-color: %1;
            border: none;
        }
    )").arg(baseBg));
}

// 计算过程按钮响应：根据当前目标类型显示对应计算页面
void ForceRequirementPanel::onCalculateProcessClicked()
{
    if (m_currentTargetId.isEmpty()) {
        qDebug() << "请先选择一个目标";
        return;
    }

    if (m_currentTargetType == "PT") {
        ui->stackedWidget->setCurrentIndex(0);
    } else if (m_currentTargetType == "AR") {
        ui->stackedWidget->setCurrentIndex(1);
    }
}

// 兵力汇总按钮响应：填充汇总表格并切换到汇总页面
void ForceRequirementPanel::onTotalForceSummaryClicked()
{
    populateSummaryTable();
    ui->stackedWidget->setCurrentIndex(2);
}

// 根据目标类型更新当前页面索引
void ForceRequirementPanel::updatePageForTarget(const QString& targetId, const QString& targetType)
{
    Q_UNUSED(targetId);
    if (targetType == "PT") {
        ui->stackedWidget->setCurrentIndex(0);
    } else if (targetType == "AR") {
        ui->stackedWidget->setCurrentIndex(1);
    }
}

// 填充兵力汇总表格：列出所有目标的毁伤要求、计算依据、建议架次等信息
void ForceRequirementPanel::populateSummaryTable()
{
    QTableWidget* table = ui->tableWidget;
    table->setRowCount(0);

    // 设置表格列头
    QStringList headers;
    headers << "目标" << "类型" << "毁伤要求" << "Pk/估目标" << "计算依据" << "建议架次" << "匹配机型";
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);

    // 表格属性设置：自适应列宽、行选中、交替行颜色
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);

    // 逐行填充目标数据（使用成员列表中的已添加目标）
    for (int i = 0; i < m_targetIds.size(); ++i) {
        int row = table->rowCount();
        table->insertRow(row);
        table->setRowHeight(row, 32);

        QString typeDisplay = (m_targetTypes[i] == "PT") ? "点目标" : "区域目标";

        QString damageStr, paramStr, basisStr, totalStr;

        if (m_targetTypes[i] == "PT") {
            PtCalcData d;
            if (m_ptResults.contains(m_targetIds[i])) {
                d = m_ptResults.value(m_targetIds[i]);
            }
            damageStr = QString("%1").arg(d.damageLevel, 0, 'f', 2);
            paramStr = QString("Pk=%1").arg(d.Pk, 0, 'f', 2);
            basisStr = QString("n=%1, 备份=%2").arg(d.n).arg(d.backup);
            totalStr = QString("%1架").arg(d.total);
        } else {
            ArCalcData d;
            if (m_arResults.contains(m_targetIds[i])) {
                d = m_arResults.value(m_targetIds[i]);
            }
            damageStr = "N/A";
            paramStr = QString("%1部").arg(d.estTargets, 0, 'f', 1);
            basisStr = QString("N搜=%1, N打=%2, 备份=%3").arg(d.N_search).arg(d.N_strike).arg(d.backup);
            totalStr = QString("%1架").arg(d.total);
        }

        auto makeItem = [](const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        table->setItem(row, 0, makeItem(QString("%1 %2").arg(m_targetIds[i]).arg(m_targetNames[i])));
        table->setItem(row, 1, makeItem(typeDisplay));
        table->setItem(row, 2, makeItem(damageStr));
        table->setItem(row, 3, makeItem(paramStr));
        table->setItem(row, 4, makeItem(basisStr));
        table->setItem(row, 5, makeItem(totalStr));
        table->setItem(row, 6, makeItem(QString("反辐射无人机")));
    }
}

// ─────────────────────────────────────────────
// QFrame 点击事件过滤
// ─────────────────────────────────────────────
// 捕获目标 pill 的鼠标点击，选中对应索引的目标并取消其他选中
bool ForceRequirementPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QPushButton *pill = qobject_cast<QPushButton*>(watched);
        if (pill) {
            int index = m_targetPills.indexOf(pill);
            if (index >= 0) {
                selectTarget(index);
                return true;
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

// ─────────────────────────────────────────────
// 目标 pill 工厂方法
// ─────────────────────────────────────────────

// 更新单个 pill 的选中/未选中样式
static void updatePillStyle(QPushButton *pill, bool selected)
{
    if (selected) {
        pill->setStyleSheet(
            "QPushButton {"
            "   background-color: #0d2466;"
            "   border: 1px solid #00b4ff;"
            "   border-radius: 6px;"
            "   color: #e0e8f0;"
            "   font-size: 12px;"
            "   text-align: left;"
            "   padding: 0px 10px;"
            "}"
        );
    } else {
        pill->setStyleSheet(
            "QPushButton {"
            "   background-color: #0b1124;"
            "   border: 1px solid #1a3a6a;"
            "   border-radius: 6px;"
            "   color: #e0e8f0;"
            "   font-size: 12px;"
            "   text-align: left;"
            "   padding: 0px 10px;"
            "}"
        );
    }
    pill->setProperty("selected", selected);
}

// 创建一个可点击的 pill 控件，使用 QPushButton 确保渲染可见
// pill 包含：类型色标 + 目标ID + 目标名称
QPushButton *ForceRequirementPanel::createTargetPill(const QString &id, const QString &name, const QString &type)
{
    QPushButton *pill = new QPushButton(ui->groupBox_Target);
    pill->setFixedHeight(36);
    pill->setCursor(Qt::PointingHandCursor);
    pill->setFlat(true);
    pill->setProperty("selected", false);
    pill->setProperty("targetPill", true);   // 标记为 target pill，跳过通用按钮样式

    // 使用 QHBoxLayout 内嵌文字
    QHBoxLayout *layout = new QHBoxLayout(pill);
    layout->setSpacing(6);
    layout->setContentsMargins(10, 4, 10, 4);

    // 类型色标（PT=青色, AR=琥珀色）
    QLabel *badge = new QLabel(pill);
    badge->setFixedSize(8, 8);
    badge->setStyleSheet(QString("background-color: %1; border-radius: 4px; border: none;")
                         .arg(type == "PT" ? "#00e5ff" : "#ffb300"));

    // 目标ID
    QLabel *idLabel = new QLabel(id, pill);
    idLabel->setStyleSheet("color: #e0e8f0; font-weight: bold; font-size: 12px; "
                           "font-family: 'Consolas'; background: transparent; border: none;");

    // 目标名称
    QLabel *nameLabel = new QLabel(name, pill);
    nameLabel->setStyleSheet("color: #7a8ba8; font-size: 11px; "
                             "font-family: 'Microsoft YaHei'; background: transparent; border: none;");

    layout->addWidget(badge);
    layout->addWidget(idLabel);
    layout->addWidget(nameLabel);
    layout->addStretch();

    updatePillStyle(pill, false);
    return pill;
}

// 更新网格行伸缩，使所有 pill 保持顶部对齐
static void updateGridRowStretch(QGridLayout *grid, int pillCount, int columns)
{
    int lastRow = (pillCount - 1) / columns;
    if (lastRow < 0) lastRow = 0;
    for (int r = 0; r <= lastRow + 1; ++r)
        grid->setRowStretch(r, 0);
    grid->setRowStretch(lastRow + 1, 1);
}

// ─────────────────────────────────────────────
// 填充默认计算目标
// ─────────────────────────────────────────────
void ForceRequirementPanel::setupDefaultTargets()
{
    addTarget("PT-01", "东郊制导雷达", "PT");
    addTarget("PT-02", "东郊火控雷达", "PT");
    addTarget("PT-03", "北山警戒雷达", "PT");
    addTarget("AR-01", "集北防空集群区", "AR");
    addTarget("AR-02", "店坪雷达走廊", "AR");
    addTarget("PT-04", "南山预警雷达", "PT");

    // 默认选中第一个
    selectTarget(0);

    // 顶部对齐
    updateGridRowStretch(m_targetGrid, m_targetPills.size(), 3);
}

// ─────────────────────────────────────────────
// 目标选择 API 实现
// ─────────────────────────────────────────────

// 添加一个计算目标 pill
int ForceRequirementPanel::addTarget(const QString &id, const QString &name, const QString &type)
{
    if (!m_targetGrid) return -1;

    int index = m_targetPills.size();
    QPushButton *pill = createTargetPill(id, name, type);

    int row = index / 3;
    int col = index % 3;
    m_targetGrid->addWidget(pill, row, col);

    // 保存到成员列表
    m_targetPills.append(pill);
    m_targetIds.append(id);
    m_targetNames.append(name);
    m_targetTypes.append(type);
    // 点击事件：通过 eventFilter 捕获 mouse press
    pill->installEventFilter(this);

    // 更新顶部对齐
    updateGridRowStretch(m_targetGrid, m_targetPills.size(), 3);

    return index;
}

// 移除指定索引的计算目标
void ForceRequirementPanel::removeTarget(int index)
{
    if (index < 0 || index >= m_targetPills.size()) return;

    // 从网格布局中移除
    QPushButton *pill = m_targetPills[index];
    m_targetGrid->removeWidget(pill);
    delete pill;
    m_targetPills.removeAt(index);
    m_targetIds.removeAt(index);
    m_targetNames.removeAt(index);
    m_targetTypes.removeAt(index);

    // 重新布局剩余 pill
    for (int i = 0; i < m_targetPills.size(); ++i) {
        int row = i / 3;
        int col = i % 3;
        m_targetGrid->addWidget(m_targetPills[i], row, col);
    }

    // 更新顶部对齐
    updateGridRowStretch(m_targetGrid, m_targetPills.size(), 3);
}

// 清空所有计算目标
void ForceRequirementPanel::clearTargets()
{
    for (QPushButton *pill : m_targetPills) {
        m_targetGrid->removeWidget(pill);
        delete pill;
    }
    m_targetPills.clear();
    m_targetIds.clear();
    m_targetNames.clear();
    m_targetTypes.clear();

    // 更新顶部对齐
    updateGridRowStretch(m_targetGrid, 0, 3);
}

// 获取计算目标总数
int ForceRequirementPanel::targetCount() const
{
    return m_targetPills.size();
}

// 选中指定索引的计算目标
void ForceRequirementPanel::selectTarget(int index)
{
    if (index < 0 || index >= m_targetPills.size()) return;

    // 取消所有选中
    for (int i = 0; i < m_targetPills.size(); ++i) {
        updatePillStyle(m_targetPills[i], i == index);
    }

    // 更新当前目标信息
    m_currentTargetId = m_targetIds[index];
    m_currentTargetType = m_targetTypes[index];

    // 切换对应模型页面
    if (m_currentTargetType == "PT") {
        ui->label_PointModel->setText(QString("点目标模型 · POINT MODEL  [ %1 ]").arg(m_currentTargetId));
        ui->stackedWidget->setCurrentIndex(0);
    } else {
        ui->label_PointModel->setText(QString("区域目标模型 · AREA MODEL  [ %1 ]").arg(m_currentTargetId));
        ui->stackedWidget->setCurrentIndex(1);
    }
}
