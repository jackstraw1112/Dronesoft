// 任务项组件实现文件

#include "TaskItemWidget.h"
#include "ui_TaskItemWidget.h"

#include <QCoreApplication>
#include <QFile>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>

TaskItemWidget::TaskItemWidget(QWidget *parent)
    : QFrame(parent), ui(new Ui::TaskItemWidget), m_taskId("")
{
    // 先构建 UI，再做参数/对象/连接初始化，便于排查初始化顺序问题。
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
    // 运行时状态字符串缓存，初始化为空。
    m_status.clear();
}

void TaskItemWidget::initObject()
{
    // 组件基础属性：对象名用于 QSS 选择器，鼠标样式强化“可点击”语义。
    setObjectName("taskItem");
    setCursor(Qt::PointingHandCursor);
    // selected/statusType 作为动态属性，供 QSS 根据状态切换视觉样式。
    setProperty("selected", false);
    ui->statusLbl->setProperty("statusType", "other");
    // 优先加载外部样式，再主动刷新一次确保属性已生效。
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
    // 样式文件路径按“运行目录 -> 构建目录回退 -> 源码目录回退”逐级尝试。
    // 这样无论从安装包运行还是开发目录运行，都能尽量命中样式文件。
    const QStringList candidates = {
            appDir + "/theme/TaskItemWidget.qss",
            appDir + "/../theme/TaskItemWidget.qss",
            appDir + "/../source/01-TaskPlan/TaskItemWidget.qss",
            "source/01-TaskPlan/TaskItemWidget.qss"};

    for (const QString &path : candidates)
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            // 命中首个可读取文件后立即使用，避免后续路径覆盖前者。
            qss = QString::fromUtf8(file.readAll());
            break;
        }
    }

    if (!qss.isEmpty())
    {
        setStyleSheet(qss);
    }
}

void TaskItemWidget::refreshStyle()
{
    // 通过 unpolish/polish 强制触发样式重算，使动态属性变更即时可见。
    style()->unpolish(this);
    style()->polish(this);
    // statusLabel 单独刷新，确保 statusType 对标签颜色/边框等样式生效。
    style()->unpolish(ui->statusLbl);
    style()->polish(ui->statusLbl);
    update();
}

void TaskItemWidget::setTaskId(QString taskId)
{
    m_taskId = taskId;
    ui->taskIdLbl->setText(taskId);
}

void TaskItemWidget::setTaskName(QString taskName)
{
    ui->taskNameLbl->setText(taskName);
}

void TaskItemWidget::setTaskStatus(QString status)
{
    // 保留原始状态值，便于后续扩展（如日志、导出或状态比较）。
    m_status = status;

    // 兼容中英文本状态输入，并统一映射到内部 statusType 样式键。
    if (status == "规划中" || status == "planning")
    {
        ui->statusLbl->setText(status == "planning" ? "规划中" : status);
        ui->statusLbl->setProperty("statusType", "planning");
    }
    else if (status == "待执行" || status == "ready")
    {
        ui->statusLbl->setText(status == "ready" ? "待执行" : status);
        ui->statusLbl->setProperty("statusType", "ready");
    }
    else if (status == "执行中" || status == "active")
    {
        ui->statusLbl->setText(status == "active" ? "执行中" : status);
        ui->statusLbl->setProperty("statusType", "active");
    }
    else if (status == "已完成" || status == "completed")
    {
        ui->statusLbl->setText(status == "completed" ? "已完成" : status);
        ui->statusLbl->setProperty("statusType", "completed");
    }
    else
    {
        // 未知状态按原文显示，并回落到 other 样式。
        ui->statusLbl->setText(status);
        ui->statusLbl->setProperty("statusType", "other");
    }

    // 状态文字/样式更新后立即刷新，避免“下一次交互才生效”。
    refreshStyle();
}

void TaskItemWidget::setTargetCount(int count)
{
    ui->targetCountLbl->setText(QString("◉ %1 目标").arg(count));
}

void TaskItemWidget::setThreatLevel(const QString &level)
{
    ui->threatLevelLbl->setText(QString("⚠ 威胁：%1").arg(level));
}

void TaskItemWidget::setTime(QString time)
{
    const QString trimmed = time.trimmed();
    // 已包含时间区间分隔符时按原文展示，否则补齐为“开始 ~ 结束”同值占位。
    if (trimmed.contains("~") || trimmed.contains("-"))
    {
        ui->timeLbl->setText(QString("⏱ %1").arg(trimmed));
    }
    else
    {
        ui->timeLbl->setText(QString("⏱ %1 ~ %1").arg(trimmed));
    }
}

void TaskItemWidget::setSelected(bool selected)
{
    // 选中态仅通过动态属性表达，具体高亮方式交给 QSS 控制。
    setProperty("selected", selected);
    refreshStyle();
}

void TaskItemWidget::mousePressEvent(QMouseEvent *event)
{
    // 左键点击发出“任务项被选中”信号，并把 taskId 作为上层唯一标识。
    if (event->button() == Qt::LeftButton)
    {
        emit clicked(m_taskId);
    }
    // 保留基类事件链，避免影响焦点/样式等默认行为。
    QFrame::mousePressEvent(event);
}
