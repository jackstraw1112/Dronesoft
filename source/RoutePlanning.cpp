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


RoutePlanning::RoutePlanning(QWidget *parent) : QFrame(parent), ui(new Ui::RoutePlanning)
{
    ui->setupUi(this);
    applyTechStyle();
}

RoutePlanning::~RoutePlanning()
{
    delete ui;
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

    ui->widget_5->setStyleSheet(QString(R"(
        QWidget#widget_5 {
            background-color: %1;
            border-top: 1px solid %2;
        }
    )").arg(panelBg).arg(borderColor));

    // 规划参数中四个阶段子面板加边框
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
}
