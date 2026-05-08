// 任务项组件实现文件

#include "TaskItemWidget.h"
#include "ui_TaskItemWidget.h"

#include <QMouseEvent>
#include <QStyle>
#include <QtGlobal>

namespace
{
    QString normalizeStatusType(const QString &status, QString *displayText)
    {
        const QString normalized = status.trimmed().toLower();

        if (normalized == QStringLiteral("规划中") || normalized == QStringLiteral("planning"))
        {
            if (displayText)
                *displayText = QStringLiteral("规划中");
            return QStringLiteral("planning");
        }
        if (normalized == QStringLiteral("待执行") || normalized == QStringLiteral("ready"))
        {
            if (displayText)
                *displayText = QStringLiteral("待执行");
            return QStringLiteral("ready");
        }
        if (normalized == QStringLiteral("执行中") || normalized == QStringLiteral("active"))
        {
            if (displayText)
                *displayText = QStringLiteral("执行中");
            return QStringLiteral("active");
        }
        if (normalized == QStringLiteral("已完成") || normalized == QStringLiteral("completed"))
        {
            if (displayText)
                *displayText = QStringLiteral("已完成");
            return QStringLiteral("completed");
        }

        if (displayText)
            *displayText = status.trimmed().isEmpty() ? QStringLiteral("未知") : status.trimmed();
        return QStringLiteral("other");
    }

    QString normalizeThreatType(const QString &level, QString *displayText)
    {
        const QString trimmed = level.trimmed();
        const QString normalized = trimmed.toLower();

        if (trimmed == QStringLiteral("高") || normalized == QStringLiteral("high"))
        {
            if (displayText)
                *displayText = QStringLiteral("高");
            return QStringLiteral("high");
        }
        if (trimmed == QStringLiteral("中") || normalized == QStringLiteral("medium"))
        {
            if (displayText)
                *displayText = QStringLiteral("中");
            return QStringLiteral("medium");
        }
        if (trimmed == QStringLiteral("低") || normalized == QStringLiteral("low"))
        {
            if (displayText)
                *displayText = QStringLiteral("低");
            return QStringLiteral("low");
        }

        if (displayText)
            *displayText = trimmed.isEmpty() ? QStringLiteral("未知") : trimmed;
        return QStringLiteral("unknown");
    }
} // namespace

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
    // QSS 选择在 theme/mission_planner_theme.qss 中命中根 QFrame（与 .ui 的 objectName 对齐）。
    // 指针光标强化“可点击”语义。
    setObjectName("frmTaskItem");
    setCursor(Qt::PointingHandCursor);
    // selected/statusType 作为动态属性，供 QSS 根据状态切换视觉样式。
    setProperty("selected", false);
    setProperty("statusType", "other");
    setProperty("threatType", "unknown");
    ui->lblStatus->setProperty("statusType", "other");
    ui->lblThreatLevel->setProperty("threatType", "unknown");
    refreshStyle();
}

void TaskItemWidget::initConnect()
{
}

void TaskItemWidget::refreshStyle()
{
    // 通过 unpolish/polish 强制触发样式重算，使动态属性变更即时可见。
    style()->unpolish(this);
    style()->polish(this);
    // statusLabel 单独刷新，确保 statusType 对标签颜色/边框等样式生效。
    style()->unpolish(ui->lblStatus);
    style()->polish(ui->lblStatus);
    style()->unpolish(ui->lblThreatLevel);
    style()->polish(ui->lblThreatLevel);
    update();
}

void TaskItemWidget::setTaskId(QString taskId)
{
    m_taskId = taskId.trimmed();
    if (m_taskId.isEmpty())
    {
        m_taskId = QStringLiteral("MSN-UNKNOWN");
    }
    ui->lblTaskId->setText(m_taskId);
}

void TaskItemWidget::setTaskName(QString taskName)
{
    const QString trimmed = taskName.trimmed();
    ui->lblTaskName->setText(trimmed.isEmpty() ? QStringLiteral("未命名任务") : trimmed);
}

void TaskItemWidget::setTaskStatus(QString status)
{
    // 保留原始状态值，便于后续扩展（如日志、导出或状态比较）。
    m_status = status.trimmed();

    QString statusText;
    const QString statusType = normalizeStatusType(m_status, &statusText);
    ui->lblStatus->setText(statusText);
    ui->lblStatus->setProperty("statusType", statusType);
    setProperty("statusType", statusType);

    // 状态文字/样式更新后立即刷新，避免“下一次交互才生效”。
    refreshStyle();
}

void TaskItemWidget::setTargetCount(int count)
{
    const int safeCount = qMax(0, count);
    ui->lblTargetCount->setText(QString("◉ %1 目标").arg(safeCount));

    if (safeCount >= 8)
        setProperty("targetDensity", "high");
    else if (safeCount >= 4)
        setProperty("targetDensity", "medium");
    else
        setProperty("targetDensity", "low");

    refreshStyle();
}

void TaskItemWidget::setThreatLevel(const QString &level)
{
    QString displayText;
    const QString threatType = normalizeThreatType(level, &displayText);
    ui->lblThreatLevel->setText(QString("⚠ 威胁：%1").arg(displayText));
    ui->lblThreatLevel->setProperty("threatType", threatType);
    setProperty("threatType", threatType);
    refreshStyle();
}

void TaskItemWidget::setTime(QString time)
{
    const QString trimmed = time.trimmed();
    if (trimmed.isEmpty())
    {
        ui->lblTime->setText(QStringLiteral("⏱ --:-- ~ --:--"));
        return;
    }

    // 已包含时间区间分隔符时按原文展示，否则补齐为“开始 ~ 结束”同值占位。
    if (trimmed.contains("~"))
    {
        ui->lblTime->setText(QString("⏱ %1").arg(trimmed));
    }
    else if (trimmed.contains(" - "))
    {
        QString normalized = trimmed;
        normalized.replace(" - ", " ~ ");
        ui->lblTime->setText(QString("⏱ %1").arg(normalized));
    }
    else
    {
        ui->lblTime->setText(QString("⏱ %1 ~ %1").arg(trimmed));
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
