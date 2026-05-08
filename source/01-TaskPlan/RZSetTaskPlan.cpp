//
// Created by Administrator on 2026/5/6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AddOrEditTaskplan.h" resolved

#include "RZSetTaskPlan.h"
#include "SetAreaTargetEditDialog.h"
#include "SetPointTargetEditDialog.h"
#include "TaskPlanningTypeConvert.h"
#include "ui_RZSetTaskPlan.h"

#include <QAction>
#include <QDateTime>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace
{
QString generateTaskId()
{
    return QStringLiteral("MSN-%1")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmsszzz")));
}

QString generatePointTargetId()
{
    return QStringLiteral("PT-%1")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmsszzz")));
}

QString generateAreaTargetId()
{
    return QStringLiteral("AR-%1")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmsszzz")));
}
} // namespace

RZSetTaskPlan::RZSetTaskPlan(QWidget *parent)
    : QWidget(parent), ui(new Ui::RZSetTaskPlan)
{
    // 先构建 UI，再按“参数 -> 对象 -> 连接”的顺序初始化，便于定位问题。
    ui->setupUi(this);

    // 初始化参数
    initParams();

    // 初始化对象
    initObject();

    // 关联信号与槽函数
    initConnect();
}

RZSetTaskPlan::~RZSetTaskPlan()
{
    delete ui;
}

void RZSetTaskPlan::initParams()
{
    // 清空运行期缓存，避免复用旧对象时带入上一次数据。
    m_pointTargets.clear();
    m_areaTargets.clear();
    // 默认只读，必须显式进入“新建/编辑”才允许修改。
    m_formEditable = false;
}

void RZSetTaskPlan::initObject()
{
    // 点目标表：整行选中、单选、只读显示；编辑通过弹窗完成。
    ui->tbwPointTarget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbwPointTarget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tbwPointTarget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbwPointTarget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tbwPointTarget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tbwPointTarget->verticalHeader()->setVisible(false);
    ui->tbwPointTarget->setContextMenuPolicy(Qt::CustomContextMenu);
    updatePointTargetActionBtnState();

    // 区域目标表：配置与点目标一致，统一交互模型。
    ui->tbwAreaTarget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbwAreaTarget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tbwAreaTarget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbwAreaTarget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tbwAreaTarget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tbwAreaTarget->verticalHeader()->setVisible(false);
    updateAreaTargetActionBtnState();

    // 默认只读，进入“新建/编辑”流程后再解锁
    setFormEditable(false);

    // 字段标签统一美化：与点目标/区域目标编辑弹窗保持一致。
    const QString fieldLabelStyle = QStringLiteral(
            "QLabel { color: #00b4ff; font-size: 10px; font-weight: 600; padding: 0 0 2px 2px; }");
    ui->lblTaskName->setStyleSheet(fieldLabelStyle);
    ui->lblTaskType->setStyleSheet(fieldLabelStyle);
    ui->lblPriority->setStyleSheet(fieldLabelStyle);
    ui->lblThreatLevelTitle->setStyleSheet(fieldLabelStyle);
    ui->lblStartTime->setStyleSheet(fieldLabelStyle);
    ui->lblEndTime->setStyleSheet(fieldLabelStyle);
    ui->lblTaskRemark->setStyleSheet(fieldLabelStyle);
}

void RZSetTaskPlan::initConnect()
{
    // 基础表单保存。
    connect(ui->btnSaveTask, &QPushButton::clicked, this, &RZSetTaskPlan::onSaveTask);

    // 点目标增删改 + 表格交互（选中、双击、右键）。
    connect(ui->btnAddPointTarget, &QPushButton::clicked, this, &RZSetTaskPlan::onAddPointTarget);
    connect(ui->btnEditPointTarget, &QPushButton::clicked, this, &RZSetTaskPlan::onEditPointTarget);
    connect(ui->btnDeletePointTarget, &QPushButton::clicked, this, &RZSetTaskPlan::onDeletePointTarget);
    connect(ui->tbwPointTarget, &QTableWidget::itemSelectionChanged, this, &RZSetTaskPlan::updatePointTargetActionBtnState);
    connect(ui->tbwPointTarget, &QTableWidget::cellDoubleClicked, this, &RZSetTaskPlan::onPointTargetDoubleClicked);
    connect(ui->tbwPointTarget, &QWidget::customContextMenuRequested, this, &RZSetTaskPlan::onPointTargetContextMenu);

    // 区域目标增删改 + 表格交互（选中、双击）。
    connect(ui->btnAddAreaTarget, &QPushButton::clicked, this, &RZSetTaskPlan::onAddAreaTarget);
    connect(ui->btnEditAreaTarget, &QPushButton::clicked, this, &RZSetTaskPlan::onEditAreaTarget);
    connect(ui->btnDeleteAreaTarget, &QPushButton::clicked, this, &RZSetTaskPlan::onDeleteAreaTarget);
    connect(ui->tbwAreaTarget, &QTableWidget::itemSelectionChanged, this, &RZSetTaskPlan::updateAreaTargetActionBtnState);
    connect(ui->tbwAreaTarget, &QTableWidget::cellDoubleClicked, this, [this](int row, int)
            {
                editAreaTargetAtRow(row);
            });
}

void RZSetTaskPlan::onSaveTask()
{
    triggerSave(true);
}

bool RZSetTaskPlan::triggerSave(bool showSuccessMessage)
{
    // ==========================
    // 第一阶段：保存入口权限校验
    // ==========================
    // 只读模式下禁止保存，避免以下风险：
    // 1) 页面处于“浏览态”时误触发写入；
    // 2) 外部状态机尚未进入编辑流程，导致数据与界面态不一致；
    // 3) 下游接收方收到“保存成功”信号后错误推进流程。
    if (!canOperate())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("当前为只读状态，请通过新建或编辑进入可操作模式。"));
        return false;
    }

    // ==========================
    // 第二阶段：读取 UI 输入快照
    // ==========================
    // 先将界面值提取到局部变量，后续校验与组装统一基于该快照，
    // 避免在函数内多次直接访问 UI 导致“读取时机不一致”。
    const QString taskName = ui->leTaskName->text().trimmed();
    QString taskId = m_lastSavedTaskInfo.taskId.trimmed();
    if (taskId.isEmpty())
    {
        taskId = generateTaskId();
    }
    const QDateTime startTime = ui->dteStartTime->dateTime();
    const QDateTime endTime = ui->dteEndTime->dateTime();
    const int taskTypeIndex = ui->cmbTaskType->currentIndex();
    const int priorityIndex = ui->cmbPriority->currentIndex();

    // ==========================
    // 第三阶段：输入有效性校验（失败即早返回）
    // ==========================
    // 校验顺序遵循“用户最容易理解/修复”的原则：
    // 名称 -> 时间 -> 枚举索引 -> 目标清单。
    // 目标清单要求：点目标和区域目标至少其一非空，防止“空任务”被保存。
    if (taskName.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入任务名称。"));
        return false;
    }
    if (startTime >= endTime)
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("任务开始时间必须早于结束时间。"));
        return false;
    }
    if (taskTypeIndex < 0 || taskTypeIndex > 1)
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择有效的任务类型。"));
        return false;
    }
    if (priorityIndex < 0 || priorityIndex > 2)
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择有效的优先级。"));
        return false;
    }
    if (m_pointTargets.isEmpty() && m_areaTargets.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请至少添加一个目标（点目标或区域目标）。"));
        return false;
    }

    // ==========================
    // 第四阶段：组装 TaskBasicInfo
    // ==========================
    // taskUid 与 taskId 职责分离：
    // - taskUid：内部稳定主键，外部不可编辑，用于跨流程稳定关联；
    // - taskId ：业务展示编号，可由用户输入/修改。
    // 这样可避免“用户改任务编号后，旧关联关系失效”的问题。
    TaskBasicInfo info;
    // 仅在“编辑已有任务”时沿用旧 taskUid；
    // 新建任务时保持空值，由上层流程/持久化层统一生成。
    if (!m_lastSavedTaskInfo.taskUid.isEmpty())
    {
        info.taskUid = m_lastSavedTaskInfo.taskUid;
    }
    info.taskName = taskName;
    info.taskId = taskId;
    info.taskType = (taskTypeIndex == 0) ? TaskType::SEAD : TaskType::DEAD;
    // 威胁等级由外部组件设定，此处仅透传，不参与编辑。
    info.overallThreatLevel = m_lastSavedTaskInfo.overallThreatLevel;

    switch (priorityIndex)
    {
        case 0:
            info.priority = PriorityLevel::P1;
            break;
        case 1:
            info.priority = PriorityLevel::P2;
            break;
        default:
            info.priority = PriorityLevel::P3;
            break;
    }

    // 时间统一转为秒级时间戳，便于：
    // 1) resolveTaskStatus 做阶段判定；
    // 2) 序列化/反序列化保持跨平台一致；
    // 3) 规避时区字符串比较导致的歧义。
    info.startTimestampSec = static_cast<uint>(startTime.toSecsSinceEpoch());
    info.endTimestampSec = static_cast<uint>(endTime.toSecsSinceEpoch());
    // 在方案阶段保存时，按“当前时间 + 起止时间”推导任务状态，
    // 保证新建与编辑路径得到一致的状态计算结果。
    info.status = resolveTaskStatus(
            TaskPlanStage::Scheme,
            static_cast<uint>(QDateTime::currentSecsSinceEpoch()),
            info.startTimestampSec,
            info.endTimestampSec);
    info.intent = ui->txeTaskRemark->toPlainText().trimmed();

    // ==========================
    // 第五阶段：缓存回写 + 多通道保存通知
    // ==========================
    // 这里保留三种粒度信号，目的是兼容历史接收方并逐步迁移：
    // 1) saveTaskClicked：仅基础信息，适配轻量监听者；
    // 2) saveTaskWithPointTargetsClicked：历史点目标通道；
    // 3) saveTaskDetailClicked：完整数据主通道（推荐）。
    //
    // 注意顺序：
    // - 先更新 m_lastSavedTaskInfo，再发信号；
    // - 确保槽函数内若回读“最近保存信息”时可拿到最新值。
    m_lastSavedTaskInfo = info;
    emit saveTaskClicked(info);
    emit saveTaskWithPointTargetsClicked(info, m_pointTargets);

    // 组装完整任务数据对象，统一承载基础信息、点目标、区域目标。
    TaskPlanningData taskData;
    taskData.basicInfo = info;
    taskData.pointTargets = m_pointTargets;
    taskData.areaTargets = m_areaTargets;
    emit saveTaskDetailClicked(taskData);
    if (showSuccessMessage)
    {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("任务保存成功。"));
    }
    return true;
}

void RZSetTaskPlan::loadTaskForEdit(const TaskBasicInfo &taskInfo, const QList<PointTargetInfo> &pointTargets, const QList<AreaTargetInfo> &areaTargets)
{
    // 进入编辑流程：先同步内存，再回填界面。
    m_lastSavedTaskInfo = taskInfo;
    m_pointTargets = pointTargets;
    m_areaTargets = areaTargets;

    ui->leTaskName->setText(taskInfo.taskName);
    ui->cmbTaskType->setCurrentIndex(taskInfo.taskType == TaskType::DEAD ? 1 : 0);

    switch (taskInfo.priority)
    {
        case PriorityLevel::P1:
            ui->cmbPriority->setCurrentIndex(0);
            break;
        case PriorityLevel::P2:
            ui->cmbPriority->setCurrentIndex(1);
            break;
        default:
            ui->cmbPriority->setCurrentIndex(2);
            break;
    }

    ui->dteStartTime->setDateTime(QDateTime::fromSecsSinceEpoch(taskInfo.startTimestampSec));
    ui->dteEndTime->setDateTime(QDateTime::fromSecsSinceEpoch(taskInfo.endTimestampSec));
    ui->txeTaskRemark->setPlainText(taskInfo.intent);

    // 威胁等级由外部组件计算，此处仅做展示。
    ui->leThreatLevel->setText(threatLevelTypeToChinese(taskInfo.overallThreatLevel));

    // 重建点目标表格（按缓存顺序回填）。
    ui->tbwPointTarget->setRowCount(0);
    for (int i = 0; i < m_pointTargets.size(); ++i)
    {
        ui->tbwPointTarget->insertRow(i);
        setPointTargetRow(i, m_pointTargets.at(i));
    }

    // 重建区域目标表格（按缓存顺序回填）。
    ui->tbwAreaTarget->setRowCount(0);
    for (int i = 0; i < m_areaTargets.size(); ++i)
    {
        ui->tbwAreaTarget->insertRow(i);
        setAreaTargetRow(i, m_areaTargets.at(i));
    }

    ui->tbwPointTarget->clearSelection();
    ui->tbwAreaTarget->clearSelection();
    // 进入编辑态后才允许操作表单控件与目标按钮。
    setFormEditable(true);
    updatePointTargetActionBtnState();
    updateAreaTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::resetForNewTask()
{
    // 新建流程入口：
    // 目标是将页面恢复到“可直接录入新任务”的干净状态，
    // 并清除上一条任务在界面与内存中的残留数据。

    // 1) 清空任务基础输入字段。
    ui->leTaskName->clear();

    // 2) 将下拉框恢复到默认选项（索引 0）。
    // cmbTaskType: 默认任务类型
    // cmbPriority: 默认优先级
    ui->cmbTaskType->setCurrentIndex(0);
    ui->cmbPriority->setCurrentIndex(0);

    // 3) 清空任务备注/意图文本，重置威胁等级展示。
    ui->txeTaskRemark->clear();
    ui->leThreatLevel->setText(QStringLiteral("--"));

    // 4) 重置时间窗口：
    // 开始时间=当前时刻，结束时间=当前+1小时。
    // 保证新建时能直接通过“开始<结束”的基础时间校验。
    const QDateTime now = QDateTime::currentDateTime();
    ui->dteStartTime->setDateTime(now);
    ui->dteEndTime->setDateTime(now.addSecs(3600));

    // 5) 清空内存缓存：
    // - m_lastSavedTaskInfo: 最近一次保存/加载的任务基础信息
    // - m_pointTargets/m_areaTargets: 目标列表数据源
    // 这样可避免新任务沿用上一任务的 taskUid 或目标数据。
    m_lastSavedTaskInfo = TaskBasicInfo();
    m_pointTargets.clear();
    m_areaTargets.clear();

    // 6) 清空两张目标表格及其选中态。
    // 表格是 m_pointTargets/m_areaTargets 的视图，需要与缓存同步清空。
    ui->tbwPointTarget->setRowCount(0);
    ui->tbwPointTarget->clearSelection();
    ui->tbwAreaTarget->setRowCount(0);
    ui->tbwAreaTarget->clearSelection();

    // 7) 进入可编辑状态（新建即编辑），并刷新按钮可用状态。
    // 例如：新增按钮可点，编辑/删除按钮取决于是否有选中行。
    setFormEditable(true);
    updatePointTargetActionBtnState();
    updateAreaTargetActionBtnState();

    // 8) 触发一次强制重绘，规避某些场景下清空后界面延迟更新的问题。
    forceRefreshView();
}

void RZSetTaskPlan::onAddPointTarget()
{
    if (!canOperate())
    {
        return;
    }

    // 打开点目标弹窗，并传入已有名称供下拉复用。
    // 这里不做全局目标库依赖，使用当前任务内名称即可满足本地编辑闭环。
    SetPointTargetEditDialog dialog(this);
    QStringList targetNames;
    for (const PointTargetInfo &target : m_pointTargets)
    {
        if (!target.name.trimmed().isEmpty() && !targetNames.contains(target.name))
        {
            targetNames.append(target.name);
        }
    }

    dialog.setTargetNameOptions(targetNames);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    PointTargetInfo targetInfo = dialog.pointTargetInfo();
    targetInfo.targetId = generatePointTargetId();
    // 以自动生成的点目标编号作为内部唯一键，避免重复插入。
    for (const PointTargetInfo &existing : m_pointTargets)
    {
        if (existing.targetId == targetInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("点目标编号重复，请重新添加。"));
            return;
        }
    }

    // 数据先入缓存，再更新表格，保持“数据源 -> 视图”的单向同步。
    m_pointTargets.append(targetInfo);
    const int row = ui->tbwPointTarget->rowCount();
    ui->tbwPointTarget->insertRow(row);
    setPointTargetRow(row, targetInfo);
    updatePointTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::onEditPointTarget()
{
    if (!canOperate())
    {
        return;
    }

    // 统一复用按行编辑逻辑，避免按钮/双击/右键三套实现分叉。
    editPointTargetAtRow(ui->tbwPointTarget->currentRow());
}

void RZSetTaskPlan::onDeletePointTarget()
{
    if (!canOperate())
    {
        return;
    }

    const int row = ui->tbwPointTarget->currentRow();
    if (row < 0 || row >= m_pointTargets.size())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的点目标。"));
        return;
    }

    // 删除前二次确认，降低误删风险。
    const QString targetName = m_pointTargets.at(row).name;
    const QMessageBox::StandardButton ret = QMessageBox::question(this, QStringLiteral("确认删除"), QStringLiteral("确定删除点目标“%1”吗？").arg(targetName));
    if (ret != QMessageBox::Yes)
    {
        return;
    }

    // 删除缓存后再删视图行，避免索引漂移引发错删。
    m_pointTargets.removeAt(row);
    ui->tbwPointTarget->removeRow(row);
    updatePointTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::updatePointTargetActionBtnState()
{
    // 编辑态 + 有选中行 才允许编辑/删除；添加仅受编辑态控制。
    const bool hasSelection = m_formEditable && (ui->tbwPointTarget->currentRow() >= 0);
    ui->btnAddPointTarget->setEnabled(m_formEditable);
    ui->btnEditPointTarget->setEnabled(hasSelection);
    ui->btnDeletePointTarget->setEnabled(hasSelection);
}

void RZSetTaskPlan::onPointTargetDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    editPointTargetAtRow(row);
}

void RZSetTaskPlan::onPointTargetContextMenu(const QPoint &pos)
{
    const int row = ui->tbwPointTarget->rowAt(pos.y());
    if (row < 0 || row >= m_pointTargets.size())
    {
        return;
    }

    ui->tbwPointTarget->selectRow(row);

    QMenu menu(this);
    QAction *editAction = menu.addAction(QStringLiteral("编辑点目标"));
    QAction *deleteAction = menu.addAction(QStringLiteral("删除点目标"));
    // 在 viewport 坐标系转全局坐标，保证菜单定位与鼠标位置一致。
    QAction *selectedAction = menu.exec(ui->tbwPointTarget->viewport()->mapToGlobal(pos));
    if (selectedAction == editAction)
    {
        editPointTargetAtRow(row);
    }

    else if (selectedAction == deleteAction)
    {
        onDeletePointTarget();
    }
}

void RZSetTaskPlan::editPointTargetAtRow(int row)
{
    if (row < 0 || row >= m_pointTargets.size())
    {
        return;
    }

    SetPointTargetEditDialog dialog(this);
    QStringList targetNames;
    for (const PointTargetInfo &target : m_pointTargets)
    {
        if (!target.name.trimmed().isEmpty() && !targetNames.contains(target.name))
        {
            targetNames.append(target.name);
        }
    }

    dialog.setTargetNameOptions(targetNames);
    dialog.setDialogTitle(QStringLiteral("编辑点目标"));
    dialog.setPointTargetInfo(m_pointTargets.at(row));
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    PointTargetInfo updatedInfo = dialog.pointTargetInfo();
    updatedInfo.targetId = m_pointTargets.at(row).targetId;

    // 编辑时保留当前行内部编号，编号不再由界面输入。
    if (updatedInfo.targetId.isEmpty())
    {
        updatedInfo.targetId = generatePointTargetId();
    }

    // 行编辑属于就地替换，保持列表顺序不变。
    m_pointTargets[row] = updatedInfo;
    setPointTargetRow(row, updatedInfo);
    forceRefreshView();
}

void RZSetTaskPlan::setPointTargetRow(int row, const PointTargetInfo &targetInfo)
{
    const QString coordinateAndCep = QStringLiteral("%1, %2")
                                             .arg(targetInfo.latitude, 0, 'f', 6)
                                             .arg(targetInfo.longitude, 0, 'f', 6);

    auto setCell = [this, row](int col, const QString &text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        ui->tbwPointTarget->setItem(row, col, item);
    };

    setCell(0, targetInfo.name);
    setCell(1, targetTypeToChinese(targetInfo.type));
    setCell(2, coordinateAndCep);
    setCell(3, targetInfo.priority);
    setCell(4, QString::number(targetInfo.requiredPk, 'f', 2));
}

void RZSetTaskPlan::onAddAreaTarget()
{
    if (!canOperate())
    {
        return;
    }

    // 弹窗内“地图拾取”先用经纬度输入模拟，后续可替换为真实地图回调。
    // 通过 requestMapPick 信号解耦：对话框不直接依赖地图模块。
    SetAreaTargetEditDialog dialog(this);
    connect(&dialog, &SetAreaTargetEditDialog::requestMapPick, this, [this, &dialog]()
            {
                bool okLat = false;
                const double lat = QInputDialog::getDouble(this, QStringLiteral("地图拾取"), QStringLiteral("请输入纬度（-90~90）："), 0.0, -90.0, 90.0, 6, &okLat);
                if (!okLat)
                {
                    return;
                }

                bool okLon = false;
                const double lon = QInputDialog::getDouble(this, QStringLiteral("地图拾取"), QStringLiteral("请输入经度（-180~180）："), 0.0, -180.0, 180.0, 6, &okLon);
                if (!okLon)
                {
                    return;
                }

                GeoPosition point;
                point.latitude = lat;
                point.longitude = lon;
                dialog.appendPickedVertex(point);
            });

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    AreaTargetInfo targetInfo = dialog.areaTargetInfo();
    targetInfo.targetId = generateAreaTargetId();

    // 数据先入缓存，再更新表格，保持”数据源 -> 视图”的单向同步。
    m_areaTargets.append(targetInfo);
    const int row = ui->tbwAreaTarget->rowCount();
    ui->tbwAreaTarget->insertRow(row);
    setAreaTargetRow(row, targetInfo);
    updateAreaTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::onEditAreaTarget()
{
    if (!canOperate())
    {
        return;
    }

    // 统一复用按行编辑逻辑，避免按钮/双击实现分叉。
    editAreaTargetAtRow(ui->tbwAreaTarget->currentRow());
}

void RZSetTaskPlan::onDeleteAreaTarget()
{
    if (!canOperate())
    {
        return;
    }

    const int row = ui->tbwAreaTarget->currentRow();
    if (row < 0 || row >= m_areaTargets.size())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的区域目标。"));
        return;
    }

    // 删除前二次确认，降低误删风险。
    const QString targetName = m_areaTargets.at(row).name;
    const QMessageBox::StandardButton ret = QMessageBox::question(this, QStringLiteral("确认删除"), QStringLiteral("确定删除区域目标“%1”吗？").arg(targetName));
    if (ret != QMessageBox::Yes)
    {
        return;
    }

    // 删除缓存后再删视图行，避免索引漂移引发错删。
    m_areaTargets.removeAt(row);
    ui->tbwAreaTarget->removeRow(row);
    updateAreaTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::updateAreaTargetActionBtnState()
{
    // 编辑态 + 有选中行 才允许编辑/删除；添加仅受编辑态控制。
    const bool hasSelection = m_formEditable && (ui->tbwAreaTarget->currentRow() >= 0);
    ui->btnAddAreaTarget->setEnabled(m_formEditable);
    ui->btnEditAreaTarget->setEnabled(hasSelection);
    ui->btnDeleteAreaTarget->setEnabled(hasSelection);
}

void RZSetTaskPlan::setAreaTargetRow(int row, const AreaTargetInfo &targetInfo)
{
    const int vertexCount = targetInfo.vertices.size();

    auto setCell = [this, row](int col, const QString &text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        ui->tbwAreaTarget->setItem(row, col, item);
    };

    setCell(0, targetInfo.targetId);
    setCell(1, targetInfo.name);
    setCell(2, areaGeometryTypeToChinese(targetInfo.areaType));
    setCell(3, QString::number(vertexCount));
}

void RZSetTaskPlan::editAreaTargetAtRow(int row)
{
    if (row < 0 || row >= m_areaTargets.size())
    {
        return;
    }

    SetAreaTargetEditDialog dialog(this);
    connect(&dialog, &SetAreaTargetEditDialog::requestMapPick, this, [this, &dialog]()
            {
                bool okLat = false;
                const double lat = QInputDialog::getDouble(
                        this, QStringLiteral("地图拾取"),
                        QStringLiteral("请输入纬度（-90~90）："),
                        0.0, -90.0, 90.0, 6, &okLat);
                if (!okLat)
                {
                    return;
                }

                bool okLon = false;
                const double lon = QInputDialog::getDouble(this, QStringLiteral("地图拾取"), QStringLiteral("请输入经度（-180~180）："), 0.0, -180.0, 180.0, 6, &okLon);
                if (!okLon)
                {
                    return;
                }

                GeoPosition point;
                point.latitude = lat;
                point.longitude = lon;
                dialog.appendPickedVertex(point);
            });
    dialog.setDialogTitle(QStringLiteral("编辑区域目标"));
    dialog.setAreaTargetInfo(m_areaTargets.at(row));
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    AreaTargetInfo updatedInfo = dialog.areaTargetInfo();
    // 编辑时保留当前行内部编号，编号不再由界面输入。
    updatedInfo.targetId = m_areaTargets.at(row).targetId;
    if (updatedInfo.targetId.isEmpty())
    {
        updatedInfo.targetId = generateAreaTargetId();
    }

    // 行编辑属于就地替换，保持列表顺序不变。
    m_areaTargets[row] = updatedInfo;
    setAreaTargetRow(row, updatedInfo);

    // 刷新界面
    forceRefreshView();
}

void RZSetTaskPlan::forceRefreshView()
{
    // 强制刷新两张表格，处理首次插入后布局延迟更新的问题。
    ui->tbwPointTarget->resizeRowsToContents();
    ui->tbwPointTarget->viewport()->update();
    ui->tbwPointTarget->updateGeometry();

    ui->tbwAreaTarget->resizeRowsToContents();
    ui->tbwAreaTarget->viewport()->update();
    ui->tbwAreaTarget->updateGeometry();

    // QWidget 级别再触发一次刷新，规避首帧布局延迟造成的显示不完整。
    update();
    repaint();
}

void RZSetTaskPlan::setFormEditable(bool editable)
{
    // 统一控制编辑态，避免局部控件状态不一致。
    m_formEditable = editable;

    // 基础信息区随编辑态统一启停，避免局部可编辑导致状态混乱。
    ui->leTaskName->setEnabled(editable);
    ui->cmbTaskType->setEnabled(editable);
    ui->cmbPriority->setEnabled(editable);
    ui->dteStartTime->setEnabled(editable);
    ui->dteEndTime->setEnabled(editable);
    ui->txeTaskRemark->setEnabled(editable);
    ui->twTarget->setEnabled(editable);
    ui->btnSaveTask->setEnabled(editable);

    updatePointTargetActionBtnState();
    updateAreaTargetActionBtnState();
}

bool RZSetTaskPlan::canOperate() const
{
    // 所有写操作统一走该开关，便于后续扩展权限控制。
    return m_formEditable;
}
