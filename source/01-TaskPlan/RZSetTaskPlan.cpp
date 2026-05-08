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
    ui->pointTargetTbw->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->pointTargetTbw->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->pointTargetTbw->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->pointTargetTbw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->pointTargetTbw->setContextMenuPolicy(Qt::CustomContextMenu);
    updatePointTargetActionBtnState();

    // 区域目标表：配置与点目标一致，统一交互模型。
    ui->areaTargetTbw->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->areaTargetTbw->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->areaTargetTbw->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->areaTargetTbw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    updateAreaTargetActionBtnState();

    // 默认只读，进入“新建/编辑”流程后再解锁
    setFormEditable(false);
}

void RZSetTaskPlan::initConnect()
{
    // 基础表单保存。
    connect(ui->saveTaskBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onSaveTask);

    // 点目标增删改 + 表格交互（选中、双击、右键）。
    connect(ui->addPointTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onAddPointTarget);
    connect(ui->editPointTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onEditPointTarget);
    connect(ui->deletePointTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onDeletePointTarget);
    connect(ui->pointTargetTbw, &QTableWidget::itemSelectionChanged, this, &RZSetTaskPlan::updatePointTargetActionBtnState);
    connect(ui->pointTargetTbw, &QTableWidget::cellDoubleClicked, this, &RZSetTaskPlan::onPointTargetDoubleClicked);
    connect(ui->pointTargetTbw, &QWidget::customContextMenuRequested, this, &RZSetTaskPlan::onPointTargetContextMenu);

    // 区域目标增删改 + 表格交互（选中、双击）。
    connect(ui->addAreaTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onAddAreaTarget);
    connect(ui->editAreaTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onEditAreaTarget);
    connect(ui->deleteAreaTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onDeleteAreaTarget);
    connect(ui->areaTargetTbw, &QTableWidget::itemSelectionChanged, this, &RZSetTaskPlan::updateAreaTargetActionBtnState);
    connect(ui->areaTargetTbw, &QTableWidget::cellDoubleClicked, this, [this](int row, int)
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
    const QString taskName = ui->taskNameEdit->text().trimmed();
    const QString taskId = ui->taskIdEdit->text().trimmed();
    const QDateTime startTime = ui->startTimeEdit->dateTime();
    const QDateTime endTime = ui->endTimeEdit->dateTime();
    const int taskTypeIndex = ui->taskTypeCombo->currentIndex();
    const int priorityIndex = ui->priorityCombo->currentIndex();

    // ==========================
    // 第三阶段：输入有效性校验（失败即早返回）
    // ==========================
    // 校验顺序遵循“用户最容易理解/修复”的原则：
    // 名称 -> 编号 -> 时间 -> 枚举索引 -> 目标清单。
    // 目标清单要求：点目标和区域目标至少其一非空，防止“空任务”被保存。
    if (taskName.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入任务名称。"));
        return false;
    }
    if (taskId.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入任务编号。"));
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
    info.intent = ui->taskRemarkEdit->toPlainText().trimmed();

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

    ui->taskNameEdit->setText(taskInfo.taskName);
    ui->taskIdEdit->setText(taskInfo.taskId);
    ui->taskTypeCombo->setCurrentIndex(taskInfo.taskType == TaskType::DEAD ? 1 : 0);

    switch (taskInfo.priority)
    {
        case PriorityLevel::P1:
            ui->priorityCombo->setCurrentIndex(0);
            break;
        case PriorityLevel::P2:
            ui->priorityCombo->setCurrentIndex(1);
            break;
        default:
            ui->priorityCombo->setCurrentIndex(2);
            break;
    }

    ui->startTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(taskInfo.startTimestampSec));
    ui->endTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(taskInfo.endTimestampSec));
    ui->taskRemarkEdit->setPlainText(taskInfo.intent);

    // 重建点目标表格（按缓存顺序回填）。
    ui->pointTargetTbw->setRowCount(0);
    for (int i = 0; i < m_pointTargets.size(); ++i)
    {
        ui->pointTargetTbw->insertRow(i);
        setPointTargetRow(i, m_pointTargets.at(i));
    }

    // 重建区域目标表格（按缓存顺序回填）。
    ui->areaTargetTbw->setRowCount(0);
    for (int i = 0; i < m_areaTargets.size(); ++i)
    {
        ui->areaTargetTbw->insertRow(i);
        setAreaTargetRow(i, m_areaTargets.at(i));
    }

    ui->pointTargetTbw->clearSelection();
    ui->areaTargetTbw->clearSelection();
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
    ui->taskNameEdit->clear();
    ui->taskIdEdit->clear();

    // 2) 将下拉框恢复到默认选项（索引 0）。
    // taskTypeCombo: 默认任务类型
    // taskTypeCombo_2: 默认优先级
    ui->taskTypeCombo->setCurrentIndex(0);
    ui->priorityCombo->setCurrentIndex(0);

    // 3) 清空任务备注/意图文本。
    ui->taskRemarkEdit->clear();

    // 4) 重置时间窗口：
    // 开始时间=当前时刻，结束时间=当前+1小时。
    // 保证新建时能直接通过“开始<结束”的基础时间校验。
    const QDateTime now = QDateTime::currentDateTime();
    ui->startTimeEdit->setDateTime(now);
    ui->endTimeEdit->setDateTime(now.addSecs(3600));

    // 5) 清空内存缓存：
    // - m_lastSavedTaskInfo: 最近一次保存/加载的任务基础信息
    // - m_pointTargets/m_areaTargets: 目标列表数据源
    // 这样可避免新任务沿用上一任务的 taskUid 或目标数据。
    m_lastSavedTaskInfo = TaskBasicInfo();
    m_pointTargets.clear();
    m_areaTargets.clear();

    // 6) 清空两张目标表格及其选中态。
    // 表格是 m_pointTargets/m_areaTargets 的视图，需要与缓存同步清空。
    ui->pointTargetTbw->setRowCount(0);
    ui->pointTargetTbw->clearSelection();
    ui->areaTargetTbw->setRowCount(0);
    ui->areaTargetTbw->clearSelection();

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

    // 保护：若目标名称来源为空，直接提示并终止，避免弹窗后下拉框为空无法提交。
    if (targetNames.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("未添加点目标名称，请先添加点目标后再执行该操作。"));
        return;
    }

    dialog.setTargetNameOptions(targetNames);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const PointTargetInfo targetInfo = dialog.pointTargetInfo();
    // 以点目标编号作为唯一键，避免重复插入。
    for (const PointTargetInfo &existing : m_pointTargets)
    {
        if (existing.targetId == targetInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("点目标编号重复，请使用其他编号。"));
            return;
        }
    }

    // 数据先入缓存，再更新表格，保持“数据源 -> 视图”的单向同步。
    m_pointTargets.append(targetInfo);
    const int row = ui->pointTargetTbw->rowCount();
    ui->pointTargetTbw->insertRow(row);
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
    editPointTargetAtRow(ui->pointTargetTbw->currentRow());
}

void RZSetTaskPlan::onDeletePointTarget()
{
    if (!canOperate())
    {
        return;
    }

    const int row = ui->pointTargetTbw->currentRow();
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
    ui->pointTargetTbw->removeRow(row);
    updatePointTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::updatePointTargetActionBtnState()
{
    // 编辑态 + 有选中行 才允许编辑/删除；添加仅受编辑态控制。
    const bool hasSelection = m_formEditable && (ui->pointTargetTbw->currentRow() >= 0);
    ui->addPointTargetBtn->setEnabled(m_formEditable);
    ui->editPointTargetBtn->setEnabled(hasSelection);
    ui->deletePointTargetBtn->setEnabled(hasSelection);
}

void RZSetTaskPlan::onPointTargetDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    editPointTargetAtRow(row);
}

void RZSetTaskPlan::onPointTargetContextMenu(const QPoint &pos)
{
    const int row = ui->pointTargetTbw->rowAt(pos.y());
    if (row < 0 || row >= m_pointTargets.size())
    {
        return;
    }

    ui->pointTargetTbw->selectRow(row);

    QMenu menu(this);
    QAction *editAction = menu.addAction(QStringLiteral("编辑点目标"));
    QAction *deleteAction = menu.addAction(QStringLiteral("删除点目标"));
    // 在 viewport 坐标系转全局坐标，保证菜单定位与鼠标位置一致。
    QAction *selectedAction = menu.exec(ui->pointTargetTbw->viewport()->mapToGlobal(pos));
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

    const PointTargetInfo updatedInfo = dialog.pointTargetInfo();

    // 编辑时跳过当前行自身，校验其余项是否存在重复编号。
    for (int i = 0; i < m_pointTargets.size(); ++i)
    {
        if (i == row)
        {
            continue;
        }
        if (m_pointTargets.at(i).targetId == updatedInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("点目标编号重复，请使用其他编号。"));
            return;
        }
    }

    // 行编辑属于就地替换，保持列表顺序不变。
    m_pointTargets[row] = updatedInfo;
    setPointTargetRow(row, updatedInfo);
    forceRefreshView();
}

void RZSetTaskPlan::setPointTargetRow(int row, const PointTargetInfo &targetInfo)
{
    // 表格展示字段统一做格式化，保证列表信息可读。
    const QString coordinateAndCep = QStringLiteral("%1, %2 / 圆概率误差:%3米")
                                             .arg(targetInfo.latitude, 0, 'f', 6)
                                             .arg(targetInfo.longitude, 0, 'f', 6)
                                             .arg(targetInfo.cepMeters, 0, 'f', 1);

    // QTableWidget 接管 item 生命周期，这里按列逐项写入展示字段。
    ui->pointTargetTbw->setItem(row, 0, new QTableWidgetItem(targetInfo.targetId));
    ui->pointTargetTbw->setItem(row, 1, new QTableWidgetItem(targetInfo.name));
    ui->pointTargetTbw->setItem(row, 2, new QTableWidgetItem(targetTypeToChinese(targetInfo.type)));
    ui->pointTargetTbw->setItem(row, 3, new QTableWidgetItem(coordinateAndCep));
    ui->pointTargetTbw->setItem(row, 4, new QTableWidgetItem(targetInfo.band));
    ui->pointTargetTbw->setItem(row, 5, new QTableWidgetItem(targetInfo.priority));
    ui->pointTargetTbw->setItem(row, 6, new QTableWidgetItem(QString::number(targetInfo.requiredPk, 'f', 2)));
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

                GeoPoint point;
                point.latitude = lat;
                point.longitude = lon;
                dialog.appendPickedVertex(point);
            });

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const AreaTargetInfo targetInfo = dialog.areaTargetInfo();
    // 以区域目标编号作为唯一键，避免重复插入。
    for (const AreaTargetInfo &existing : m_areaTargets)
    {
        if (existing.targetId == targetInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("区域目标编号重复，请使用其他编号。"));
            return;
        }
    }

    // 数据先入缓存，再更新表格，保持“数据源 -> 视图”的单向同步。
    m_areaTargets.append(targetInfo);
    const int row = ui->areaTargetTbw->rowCount();
    ui->areaTargetTbw->insertRow(row);
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
    editAreaTargetAtRow(ui->areaTargetTbw->currentRow());
}

void RZSetTaskPlan::onDeleteAreaTarget()
{
    if (!canOperate())
    {
        return;
    }

    const int row = ui->areaTargetTbw->currentRow();
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
    ui->areaTargetTbw->removeRow(row);
    updateAreaTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::updateAreaTargetActionBtnState()
{
    // 编辑态 + 有选中行 才允许编辑/删除；添加仅受编辑态控制。
    const bool hasSelection = m_formEditable && (ui->areaTargetTbw->currentRow() >= 0);
    ui->addAreaTargetBtn->setEnabled(m_formEditable);
    ui->editAreaTargetBtn->setEnabled(hasSelection);
    ui->deleteAreaTargetBtn->setEnabled(hasSelection);
}

void RZSetTaskPlan::setAreaTargetRow(int row, const AreaTargetInfo &targetInfo)
{
    const QString centerCoordinate = QStringLiteral("%1, %2").arg(targetInfo.centerLatitude, 0, 'f', 6).arg(targetInfo.centerLongitude, 0, 'f', 6);
    const QString areaRange = QStringLiteral("%1 km").arg(targetInfo.radiusKm, 0, 'f', 2);

    // QTableWidget 接管 item 生命周期，这里按列逐项写入展示字段。
    ui->areaTargetTbw->setItem(row, 0, new QTableWidgetItem(targetInfo.targetId));
    ui->areaTargetTbw->setItem(row, 1, new QTableWidgetItem(targetInfo.name));
    ui->areaTargetTbw->setItem(row, 2, new QTableWidgetItem(areaGeometryTypeToChinese(targetInfo.areaType)));
    ui->areaTargetTbw->setItem(row, 3, new QTableWidgetItem(centerCoordinate));
    ui->areaTargetTbw->setItem(row, 4, new QTableWidgetItem(areaRange));
    ui->areaTargetTbw->setItem(row, 5, new QTableWidgetItem(targetInfo.expectedEmitters));
    ui->areaTargetTbw->setItem(row, 6, new QTableWidgetItem(searchStrategyTypeToChinese(targetInfo.searchStrategy)));
    ui->areaTargetTbw->setItem(row, 7, new QTableWidgetItem(priorityToChinese(targetInfo.priority)));
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

                GeoPoint point;
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

    const AreaTargetInfo updatedInfo = dialog.areaTargetInfo();
    // 编辑时跳过当前行自身，校验其余项是否存在重复编号。
    for (int i = 0; i < m_areaTargets.size(); ++i)
    {
        if (i == row)
        {
            continue;
        }
        if (m_areaTargets.at(i).targetId == updatedInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("区域目标编号重复，请使用其他编号。"));
            return;
        }
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
    ui->pointTargetTbw->resizeRowsToContents();
    ui->pointTargetTbw->viewport()->update();
    ui->pointTargetTbw->updateGeometry();

    ui->areaTargetTbw->resizeRowsToContents();
    ui->areaTargetTbw->viewport()->update();
    ui->areaTargetTbw->updateGeometry();

    // QWidget 级别再触发一次刷新，规避首帧布局延迟造成的显示不完整。
    update();
    repaint();
}

void RZSetTaskPlan::setFormEditable(bool editable)
{
    // 统一控制编辑态，避免局部控件状态不一致。
    m_formEditable = editable;

    // 基础信息区随编辑态统一启停，避免局部可编辑导致状态混乱。
    ui->taskNameEdit->setEnabled(editable);
    ui->taskIdEdit->setEnabled(editable);
    ui->taskTypeCombo->setEnabled(editable);
    ui->priorityCombo->setEnabled(editable);
    ui->startTimeEdit->setEnabled(editable);
    ui->endTimeEdit->setEnabled(editable);
    ui->taskRemarkEdit->setEnabled(editable);
    ui->targetTw->setEnabled(editable);
    ui->saveTaskBtn->setEnabled(editable);

    updatePointTargetActionBtnState();
    updateAreaTargetActionBtnState();
}

bool RZSetTaskPlan::canOperate() const
{
    // 所有写操作统一走该开关，便于后续扩展权限控制。
    return m_formEditable;
}
