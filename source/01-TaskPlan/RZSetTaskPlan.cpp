//
// Created by Administrator on 2026/5/6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AddOrEditTaskplan.h" resolved

#include "RZSetTaskPlan.h"
#include "../TaskPlanningTypeConvert.h"
#include "SetAreaTargetEditDialog.h"
#include "SetPointTargetEditDialog.h"
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
    m_pointTargets.clear();
    m_areaTargets.clear();
    m_formEditable = false;
}

void RZSetTaskPlan::initObject()
{
    ui->tableWidget_2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_2->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget_2->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_2->setContextMenuPolicy(Qt::CustomContextMenu);
    updatePointTargetActionBtnState();

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    updateAreaTargetActionBtnState();

    // 默认只读，进入“新建/编辑”流程后再解锁
    setFormEditable(false);
}

void RZSetTaskPlan::initConnect()
{
    connect(ui->saveTaskBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onSaveTaskBtnClicked);
    connect(ui->addPointTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onAddPointTargetBtnClicked);
    connect(ui->editPointTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onEditPointTargetBtnClicked);
    connect(ui->deletePointTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onDeletePointTargetBtnClicked);
    connect(ui->tableWidget_2, &QTableWidget::itemSelectionChanged, this, &RZSetTaskPlan::updatePointTargetActionBtnState);
    connect(ui->tableWidget_2, &QTableWidget::cellDoubleClicked, this, &RZSetTaskPlan::onPointTargetDoubleClicked);
    connect(ui->tableWidget_2, &QWidget::customContextMenuRequested, this, &RZSetTaskPlan::onPointTargetContextMenu);

    connect(ui->addAreaTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onAddAreaTargetBtnClicked);
    connect(ui->editAreaTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onEditAreaTargetBtnClicked);
    connect(ui->deleteAreaTargetBtn, &QPushButton::clicked, this, &RZSetTaskPlan::onDeleteAreaTargetBtnClicked);
    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, this, &RZSetTaskPlan::updateAreaTargetActionBtnState);
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, [this](int row, int)
            { editAreaTargetAtRow(row); });
}

void RZSetTaskPlan::onSaveTaskBtnClicked()
{
    triggerSave(true);
}

bool RZSetTaskPlan::triggerSave(bool showSuccessMessage)
{
    if (!canOperate())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("当前为只读状态，请通过新建或编辑进入可操作模式。"));
        return false;
    }

    const QString taskName = ui->taskNameEdit->text().trimmed();
    const QString taskId = ui->taskNameEdit_2->text().trimmed();
    const QDateTime startTime = ui->startTimeEdit->dateTime();
    const QDateTime endTime = ui->endTimeEdit->dateTime();
    const int taskTypeIndex = ui->taskTypeCombo->currentIndex();
    const int priorityIndex = ui->taskTypeCombo_2->currentIndex();

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

    TaskBasicInfo info;
    // 新建任务使用自动生成的 taskUid；编辑任务沿用原 taskUid。
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

    info.startTimestampSec = static_cast<uint>(startTime.toSecsSinceEpoch());
    info.endTimestampSec = static_cast<uint>(endTime.toSecsSinceEpoch());
    info.status = resolveTaskStatus(
            TaskPlanStage::Scheme,
            static_cast<uint>(QDateTime::currentSecsSinceEpoch()),
            info.startTimestampSec,
            info.endTimestampSec);
    info.intent = ui->textEdit->toPlainText().trimmed();

    m_lastSavedTaskInfo = info;
    emit saveTaskClicked(info);
    emit saveTaskWithPointTargetsClicked(info, m_pointTargets);

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

void RZSetTaskPlan::loadTaskForEdit(const TaskBasicInfo &taskInfo,
                                    const QList<PointTargetInfo> &pointTargets,
                                    const QList<AreaTargetInfo> &areaTargets)
{
    m_lastSavedTaskInfo = taskInfo;
    m_pointTargets = pointTargets;
    m_areaTargets = areaTargets;

    ui->taskNameEdit->setText(taskInfo.taskName);
    ui->taskNameEdit_2->setText(taskInfo.taskId);
    ui->taskTypeCombo->setCurrentIndex(taskInfo.taskType == TaskType::DEAD ? 1 : 0);

    switch (taskInfo.priority)
    {
        case PriorityLevel::P1:
            ui->taskTypeCombo_2->setCurrentIndex(0);
            break;
        case PriorityLevel::P2:
            ui->taskTypeCombo_2->setCurrentIndex(1);
            break;
        default:
            ui->taskTypeCombo_2->setCurrentIndex(2);
            break;
    }

    ui->startTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(taskInfo.startTimestampSec));
    ui->endTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(taskInfo.endTimestampSec));
    ui->textEdit->setPlainText(taskInfo.intent);

    ui->tableWidget_2->setRowCount(0);
    for (int i = 0; i < m_pointTargets.size(); ++i)
    {
        ui->tableWidget_2->insertRow(i);
        setPointTargetRow(i, m_pointTargets.at(i));
    }

    ui->tableWidget->setRowCount(0);
    for (int i = 0; i < m_areaTargets.size(); ++i)
    {
        ui->tableWidget->insertRow(i);
        setAreaTargetRow(i, m_areaTargets.at(i));
    }

    ui->tableWidget_2->clearSelection();
    ui->tableWidget->clearSelection();
    setFormEditable(true);
    updatePointTargetActionBtnState();
    updateAreaTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::resetForNewTask()
{
    ui->taskNameEdit->clear();
    ui->taskNameEdit_2->clear();
    ui->taskTypeCombo->setCurrentIndex(0);
    ui->taskTypeCombo_2->setCurrentIndex(0);
    ui->textEdit->clear();

    const QDateTime now = QDateTime::currentDateTime();
    ui->startTimeEdit->setDateTime(now);
    ui->endTimeEdit->setDateTime(now.addSecs(3600));

    m_lastSavedTaskInfo = TaskBasicInfo();
    m_pointTargets.clear();
    m_areaTargets.clear();
    ui->tableWidget_2->setRowCount(0);
    ui->tableWidget_2->clearSelection();
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->clearSelection();
    setFormEditable(true);
    updatePointTargetActionBtnState();
    updateAreaTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::onAddPointTargetBtnClicked()
{
    if (!canOperate())
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
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const PointTargetInfo targetInfo = dialog.pointTargetInfo();
    for (const PointTargetInfo &existing : m_pointTargets)
    {
        if (existing.targetId == targetInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"),
                                 QStringLiteral("点目标编号重复，请使用其他编号。"));
            return;
        }
    }

    m_pointTargets.append(targetInfo);
    const int row = ui->tableWidget_2->rowCount();
    ui->tableWidget_2->insertRow(row);
    setPointTargetRow(row, targetInfo);
    updatePointTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::onEditPointTargetBtnClicked()
{
    if (!canOperate())
    {
        return;
    }
    editPointTargetAtRow(ui->tableWidget_2->currentRow());
}

void RZSetTaskPlan::onDeletePointTargetBtnClicked()
{
    if (!canOperate())
    {
        return;
    }

    const int row = ui->tableWidget_2->currentRow();
    if (row < 0 || row >= m_pointTargets.size())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的点目标。"));
        return;
    }

    const QString targetName = m_pointTargets.at(row).name;
    const QMessageBox::StandardButton ret = QMessageBox::question(
            this,
            QStringLiteral("确认删除"),
            QStringLiteral("确定删除点目标“%1”吗？").arg(targetName));
    if (ret != QMessageBox::Yes)
    {
        return;
    }

    m_pointTargets.removeAt(row);
    ui->tableWidget_2->removeRow(row);
    updatePointTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::updatePointTargetActionBtnState()
{
    const bool hasSelection = m_formEditable && (ui->tableWidget_2->currentRow() >= 0);
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
    const int row = ui->tableWidget_2->rowAt(pos.y());
    if (row < 0 || row >= m_pointTargets.size())
    {
        return;
    }

    ui->tableWidget_2->selectRow(row);

    QMenu menu(this);
    QAction *editAction = menu.addAction(QStringLiteral("编辑点目标"));
    QAction *deleteAction = menu.addAction(QStringLiteral("删除点目标"));
    QAction *selectedAction = menu.exec(ui->tableWidget_2->viewport()->mapToGlobal(pos));
    if (selectedAction == editAction)
    {
        editPointTargetAtRow(row);
    }
    else if (selectedAction == deleteAction)
    {
        onDeletePointTargetBtnClicked();
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
    for (int i = 0; i < m_pointTargets.size(); ++i)
    {
        if (i == row)
        {
            continue;
        }
        if (m_pointTargets.at(i).targetId == updatedInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"),
                                 QStringLiteral("点目标编号重复，请使用其他编号。"));
            return;
        }
    }

    m_pointTargets[row] = updatedInfo;
    setPointTargetRow(row, updatedInfo);
    forceRefreshView();
}

void RZSetTaskPlan::setPointTargetRow(int row, const PointTargetInfo &targetInfo)
{
    const QString coordinateAndCep = QStringLiteral("%1, %2 / CEP:%3m")
                                             .arg(targetInfo.latitude, 0, 'f', 6)
                                             .arg(targetInfo.longitude, 0, 'f', 6)
                                             .arg(targetInfo.cepMeters, 0, 'f', 1);

    ui->tableWidget_2->setItem(row, 0, new QTableWidgetItem(targetInfo.targetId));
    ui->tableWidget_2->setItem(row, 1, new QTableWidgetItem(targetInfo.name));
    ui->tableWidget_2->setItem(row, 2, new QTableWidgetItem(targetTypeToChinese(targetInfo.type)));
    ui->tableWidget_2->setItem(row, 3, new QTableWidgetItem(coordinateAndCep));
    ui->tableWidget_2->setItem(row, 4, new QTableWidgetItem(targetInfo.band));
    ui->tableWidget_2->setItem(row, 5, new QTableWidgetItem(targetInfo.priority));
    ui->tableWidget_2->setItem(row, 6, new QTableWidgetItem(QString::number(targetInfo.requiredPk, 'f', 2)));
}

void RZSetTaskPlan::onAddAreaTargetBtnClicked()
{
    if (!canOperate())
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
            0.0, -90.0, 90.0, 6, &okLat
        );
        if (!okLat) {
            return;
        }

        bool okLon = false;
        const double lon = QInputDialog::getDouble(
            this, QStringLiteral("地图拾取"),
            QStringLiteral("请输入经度（-180~180）："),
            0.0, -180.0, 180.0, 6, &okLon
        );
        if (!okLon) {
            return;
        }

        GeoPoint point;
        point.latitude = lat;
        point.longitude = lon;
        dialog.appendPickedVertex(point); });

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const AreaTargetInfo targetInfo = dialog.areaTargetInfo();
    for (const AreaTargetInfo &existing : m_areaTargets)
    {
        if (existing.targetId == targetInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"),
                                 QStringLiteral("区域目标编号重复，请使用其他编号。"));
            return;
        }
    }

    m_areaTargets.append(targetInfo);
    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    setAreaTargetRow(row, targetInfo);
    updateAreaTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::onEditAreaTargetBtnClicked()
{
    if (!canOperate())
    {
        return;
    }
    editAreaTargetAtRow(ui->tableWidget->currentRow());
}

void RZSetTaskPlan::onDeleteAreaTargetBtnClicked()
{
    if (!canOperate())
    {
        return;
    }

    const int row = ui->tableWidget->currentRow();
    if (row < 0 || row >= m_areaTargets.size())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择要删除的区域目标。"));
        return;
    }

    const QString targetName = m_areaTargets.at(row).name;
    const QMessageBox::StandardButton ret = QMessageBox::question(
            this,
            QStringLiteral("确认删除"),
            QStringLiteral("确定删除区域目标“%1”吗？").arg(targetName));
    if (ret != QMessageBox::Yes)
    {
        return;
    }

    m_areaTargets.removeAt(row);
    ui->tableWidget->removeRow(row);
    updateAreaTargetActionBtnState();
    forceRefreshView();
}

void RZSetTaskPlan::updateAreaTargetActionBtnState()
{
    const bool hasSelection = m_formEditable && (ui->tableWidget->currentRow() >= 0);
    ui->addAreaTargetBtn->setEnabled(m_formEditable);
    ui->editAreaTargetBtn->setEnabled(hasSelection);
    ui->deleteAreaTargetBtn->setEnabled(hasSelection);
}

void RZSetTaskPlan::setAreaTargetRow(int row, const AreaTargetInfo &targetInfo)
{
    const QString centerCoordinate = QStringLiteral("%1, %2")
                                             .arg(targetInfo.centerLatitude, 0, 'f', 6)
                                             .arg(targetInfo.centerLongitude, 0, 'f', 6);
    const QString areaRange = QStringLiteral("%1 km").arg(targetInfo.radiusKm, 0, 'f', 2);

    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(targetInfo.targetId));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(targetInfo.name));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(areaGeometryTypeToChinese(targetInfo.areaType)));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(centerCoordinate));
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem(areaRange));
    ui->tableWidget->setItem(row, 5, new QTableWidgetItem(targetInfo.expectedEmitters));
    ui->tableWidget->setItem(row, 6, new QTableWidgetItem(searchStrategyTypeToChinese(targetInfo.searchStrategy)));
    ui->tableWidget->setItem(row, 7, new QTableWidgetItem(priorityToChinese(targetInfo.priority)));
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
            0.0, -90.0, 90.0, 6, &okLat
        );
        if (!okLat) {
            return;
        }

        bool okLon = false;
        const double lon = QInputDialog::getDouble(
            this, QStringLiteral("地图拾取"),
            QStringLiteral("请输入经度（-180~180）："),
            0.0, -180.0, 180.0, 6, &okLon
        );
        if (!okLon) {
            return;
        }

        GeoPoint point;
        point.latitude = lat;
        point.longitude = lon;
        dialog.appendPickedVertex(point); });
    dialog.setDialogTitle(QStringLiteral("编辑区域目标"));
    dialog.setAreaTargetInfo(m_areaTargets.at(row));
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const AreaTargetInfo updatedInfo = dialog.areaTargetInfo();
    for (int i = 0; i < m_areaTargets.size(); ++i)
    {
        if (i == row)
        {
            continue;
        }
        if (m_areaTargets.at(i).targetId == updatedInfo.targetId)
        {
            QMessageBox::warning(this, QStringLiteral("提示"),
                                 QStringLiteral("区域目标编号重复，请使用其他编号。"));
            return;
        }
    }

    m_areaTargets[row] = updatedInfo;
    setAreaTargetRow(row, updatedInfo);
    forceRefreshView();
}

void RZSetTaskPlan::forceRefreshView()
{
    ui->tableWidget_2->resizeRowsToContents();
    ui->tableWidget_2->viewport()->update();
    ui->tableWidget_2->updateGeometry();

    ui->tableWidget->resizeRowsToContents();
    ui->tableWidget->viewport()->update();
    ui->tableWidget->updateGeometry();

    update();
    repaint();
}

void RZSetTaskPlan::setFormEditable(bool editable)
{
    m_formEditable = editable;

    ui->taskNameEdit->setEnabled(editable);
    ui->taskNameEdit_2->setEnabled(editable);
    ui->taskTypeCombo->setEnabled(editable);
    ui->taskTypeCombo_2->setEnabled(editable);
    ui->startTimeEdit->setEnabled(editable);
    ui->endTimeEdit->setEnabled(editable);
    ui->textEdit->setEnabled(editable);
    ui->tabWidget->setEnabled(editable);
    ui->saveTaskBtn->setEnabled(editable);

    updatePointTargetActionBtnState();
    updateAreaTargetActionBtnState();
}

bool RZSetTaskPlan::canOperate() const
{
    return m_formEditable;
}
