// 任务项组件实现文件

#include "TaskItemWidget.h"
#include "ui_TaskItemWidget.h"

#include <QCoreApplication>
#include <QFile>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>

TaskItemWidget::TaskItemWidget(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::TaskItemWidget)
    , m_taskId("")
{
    ui->setupUi(this);

    // 初始化参数
    initParams();

    // 初始化对象
    initObject();

    // 关联信号与槽函数
    initConnect();
}

TaskItemWidget::~TaskItemWidget()
{
    delete ui;
}

void TaskItemWidget::initParams()
{
    m_status.clear();
}

void TaskItemWidget::initObject()
{
    setObjectName("taskItem");
    setCursor(Qt::PointingHandCursor);
    setProperty("selected", false);
    ui->statusLabel->setProperty("statusType", "other");
    loadStyleSheet();
    refreshStyle();
}

void TaskItemWidget::initConnect()
{
}

void TaskItemWidget::loadStyleSheet()
{
    QString qss;
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + "/theme/TaskItemWidget.qss",
        appDir + "/../theme/TaskItemWidget.qss",
        appDir + "/../source/01-TaskPlan/TaskItemWidget.qss",
        "source/01-TaskPlan/TaskItemWidget.qss"
    };

    for (const QString &path : candidates) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qss = QString::fromUtf8(file.readAll());
            break;
        }
    }

    if (!qss.isEmpty()) {
        setStyleSheet(qss);
    }
}

void TaskItemWidget::refreshStyle()
{
    style()->unpolish(this);
    style()->polish(this);
    style()->unpolish(ui->statusLabel);
    style()->polish(ui->statusLabel);
    update();
}

void TaskItemWidget::setTaskId(QString taskId)
{
    m_taskId = taskId;
    ui->taskIdLabel->setText(taskId);
}

void TaskItemWidget::setTaskName(QString taskName)
{
    ui->taskNameLabel->setText(taskName);
}

void TaskItemWidget::setTaskStatus(QString status)
{
    m_status = status;

    if (status == "规划中" || status == "planning") {
        ui->statusLabel->setText(status == "planning" ? "规划中" : status);
        ui->statusLabel->setProperty("statusType", "planning");
    } else if (status == "待执行" || status == "ready") {
        ui->statusLabel->setText(status == "ready" ? "待执行" : status);
        ui->statusLabel->setProperty("statusType", "ready");
    } else if (status == "执行中" || status == "active") {
        ui->statusLabel->setText(status == "active" ? "执行中" : status);
        ui->statusLabel->setProperty("statusType", "active");
    } else if (status == "已完成" || status == "completed") {
        ui->statusLabel->setText(status == "completed" ? "已完成" : status);
        ui->statusLabel->setProperty("statusType", "completed");
    } else {
        ui->statusLabel->setText(status);
        ui->statusLabel->setProperty("statusType", "other");
    }

    refreshStyle();
}

void TaskItemWidget::setTargetCount(int count)
{
    ui->targetCountLabel->setText(QString("◉ %1 目标").arg(count));
}

void TaskItemWidget::setThreatLevel(const QString &level)
{
    ui->threatLevelLabel->setText(QString("⚠ 威胁：%1").arg(level));
}

void TaskItemWidget::setTime(QString time)
{
    const QString trimmed = time.trimmed();
    if (trimmed.contains("~") || trimmed.contains("-")) {
        ui->timeLabel->setText(QString("⏱ %1").arg(trimmed));
    } else {
        ui->timeLabel->setText(QString("⏱ %1 ~ %1").arg(trimmed));
    }
}

void TaskItemWidget::setSelected(bool selected)
{
    setProperty("selected", selected);
    refreshStyle();
}

void TaskItemWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_taskId);
    }
    QFrame::mousePressEvent(event);
}
