#include "PathDisplayDialog.h"
#include "ui_PathDisplayDialog.h"
#include <QAbstractItemView>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QWidget>

/**
 * @brief 构造函数
 * @param parent 父窗口指针
 * @details 初始化UI，设置路径点表格属性
 */
PathDisplayDialog::PathDisplayDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PathDisplayDialog)
{
    ui->setupUi(this);

    InitDisplay();

    
    
}

/**
 * @brief 析构函数
 * @details 释放UI资源
 */
PathDisplayDialog::~PathDisplayDialog()
{
    delete ui;
}

void PathDisplayDialog::InitDisplay()
{
    // 配置路径点表格
    ui->pathPointTable->horizontalHeader()->setStretchLastSection(false);
    ui->pathPointTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->pathPointTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->pathPointTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->pathPointTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->pathPointTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    ui->pathPointTable->horizontalHeader()->resizeSection(4, 160);
    ui->pathPointTable->verticalHeader()->setVisible(false);
    ui->pathPointTable->verticalHeader()->setDefaultSectionSize(45);
    ui->pathPointTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->pathPointTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->pathPointTable->setAlternatingRowColors(false);

    // 连接关闭按钮信号
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->closeButton2, &QPushButton::clicked, this, &QDialog::reject);

}

/**
 * @brief 设置路径数据
 * @param path 路径规划信息
 * @details 显示无人机名称、关联任务、路径点数，并将路径点添加到表格
 */
void PathDisplayDialog::setPathData(const PathPlanning& path)
{
    // 显示基本信息
    ui->uavNameValue->setText(path.uavName);
    ui->taskValue->setText(path.relatedTask);
    ui->pointCountValue->setText(QString::number(path.pathPointCount));

    // 清空并重新填充路径点表格
    ui->pathPointTable->setRowCount(0);

    for (int i = 0; i < path.fightPathPoints.size(); ++i) {
        const PathPoint& point = path.fightPathPoints.at(i);
        int row = ui->pathPointTable->rowCount();
        ui->pathPointTable->insertRow(row);

        // 序号
        QTableWidgetItem *orderItem = new QTableWidgetItem(QString::number(point.pointOrder));
        orderItem->setTextAlignment(Qt::AlignCenter);
        ui->pathPointTable->setItem(row, 0, orderItem);
        // 纬度（保留6位小数）
        QTableWidgetItem *latItem = new QTableWidgetItem(QString::number(point.latitude, 'f', 6));
        latItem->setTextAlignment(Qt::AlignCenter);
        ui->pathPointTable->setItem(row, 1, latItem);
        // 经度（保留6位小数）
        QTableWidgetItem *lonItem = new QTableWidgetItem(QString::number(point.longitude, 'f', 6));
        lonItem->setTextAlignment(Qt::AlignCenter);
        ui->pathPointTable->setItem(row, 2, lonItem);
        // 高度（保留1位小数）
        QTableWidgetItem *altItem = new QTableWidgetItem(QString::number(point.altitude, 'f', 1));
        altItem->setTextAlignment(Qt::AlignCenter);
        ui->pathPointTable->setItem(row, 3, altItem);

        // 操作栏：地图选点 + 删除
        QWidget *actionWidget = new QWidget();
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(4, 4, 4, 4);
        actionLayout->setSpacing(4);

        QPushButton *mapBtn = new QPushButton(QString::fromUtf8("\xe5\x9c\xb0\xe5\x9b\xbe\xe9\x80\x89\xe7\x82\xb9"));
        mapBtn->setFixedHeight(18);
        mapBtn->setFixedWidth(70);
        mapBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #0f1a2e;"
            "  border: 1px solid #1a3a6a;"
            "  border-radius: 2px;"
            "  color: #00b4ff;"
            "  padding: 0px 4px;"
            "  font-size: 10px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #0f1a3e;"
            "  border: 1px solid #00b4ff;"
            "  color: #00e5ff;"
            "}"
        ));

        QPushButton *delBtn = new QPushButton(QString::fromUtf8("\xe5\x88\xa0\xe9\x99\xa4"));
        delBtn->setFixedHeight(18);
        delBtn->setFixedWidth(70);
        delBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #2a0f0f;"
            "  border: 1px solid #6a1a1a;"
            "  border-radius: 2px;"
            "  color: #ff6b6b;"
            "  padding: 0px 4px;"
            "  font-size: 10px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #3a1515;"
            "  border: 1px solid #ff4444;"
            "  color: #ff8888;"
            "}"
        ));
        connect(delBtn, &QPushButton::clicked, this, [this, row]() {
            ui->pathPointTable->removeRow(row);
        });

        actionLayout->addWidget(mapBtn);
        actionLayout->addWidget(delBtn);
        ui->pathPointTable->setCellWidget(row, 4, actionWidget);
    }
}
