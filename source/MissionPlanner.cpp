// 反辐射无人机任务规划系统 - 主窗口实现文件
// ARUA Mission Planning System - Main Window Implementation

#include "MissionPlanner.h"
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include "TaskPlanningTypeConvert.h"

MissionPlanner *MissionPlanner::GetInstance(QWidget *parent)
{
    // Meyers Singleton：函数内静态实例，避免手动管理生命周期
    static MissionPlanner instance(parent);
    return &instance;
}

MissionPlanner::MissionPlanner(QWidget *parent)
    : QMainWindow(parent), ui(new Ui_MissionPlanner)
{
    ui->setupUi(this);

    // 初始化参数
    initParams();

    // 初始化对象
    initObject();

    // 关联信号与槽函数
    initConnect();
}

MissionPlanner::~MissionPlanner()
{
    delete ui;
}

void MissionPlanner::initParams()
{
    // 清空运行时缓存，确保每次启动都是干净状态
    m_taskStore.clear();
    m_taskNumberToUid.clear();
    m_currentEditingTaskUid.clear();
    m_runtimeStage = TaskPlanStage::Scheme;
    m_runtimeSimTimestampSec = static_cast<uint>(QDateTime::currentSecsSinceEpoch());
}

void MissionPlanner::initObject()
{
#if WIN32
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Window);	//窗口头部隐藏
#else
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog); // 窗口头部隐藏
#endif
    // setAttribute(Qt::WA_TranslucentBackground, false);		//窗口透明
    // setWindowOpacity(0.85);
    // setAutoFillBackground(true);

    // 创建步骤导航按钮组，包含5个步骤按钮（任务创建、兵力需求计算、协同任务分配、航路规划、参数装订）
    stepButtonGroup = new QButtonGroup(this);
    stepButtonGroup->addButton(ui->step1Btn, 0); // 步骤1：任务创建
    stepButtonGroup->addButton(ui->step2Btn, 1); // 步骤2：兵力需求计算
    stepButtonGroup->addButton(ui->step3Btn, 2); // 步骤3：协同任务分配
    stepButtonGroup->addButton(ui->step4Btn, 3); // 步骤4：航路规划
    stepButtonGroup->addButton(ui->step5Btn, 4); // 步骤5：参数装订
    stepButtonGroup->setExclusive(true);         // 设置按钮互斥（单选）

    // 默认显示第一个步骤页面（任务创建）
    ui->contentStackedWidget->setCurrentIndex(0);
    ui->step1Btn->setChecked(true);
    // 缓存常用子控件指针，减少后续重复访问 ui
    mTaskListWidget = ui->widget_Task;
    mRightSidePanel = ui->widget_Source;

    // 延时初始化临时演示数据，避免界面初次布局未完成导致刷新不完整
    QTimer::singleShot(0, this, [this]()
                       {
        generateDemoData();
        refreshTaskList();

        if (!m_taskStore.isEmpty())
        {
            const QString firstTaskId = m_taskStore.first().basicInfo.taskId;
            if (!firstTaskId.isEmpty())
            {
                onTaskItemSelected(firstTaskId);
            }
        } });
}

// 关联信号与槽函数
void MissionPlanner::initConnect()
{
    // 步骤导航按钮切换信号 -> onStepChanged槽函数
    connect(stepButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onStepChanged(int)));

    if (mTaskListWidget)
    {
        // 左侧任务列表的新增/删除/选中事件统一在主窗口处理
        connect(mTaskListWidget, &RZTaskListWidget::newTaskClicked, this, &MissionPlanner::onNewTaskClicked);
        connect(mTaskListWidget, &RZTaskListWidget::deleteTaskClicked, this, &MissionPlanner::onDeleteTaskClicked);
        connect(mTaskListWidget, &RZTaskListWidget::taskSelected, this, &MissionPlanner::onTaskItemSelected);
    }

    // 步骤1页面保存后，把完整任务数据写回主窗口缓存
    connect(ui->step1Page, &RZSetTaskPlan::saveTaskDetailClicked, this, &MissionPlanner::onTaskSavedDetail);
}

// 步骤导航切换事件处理
void MissionPlanner::onStepChanged(int index)
{
    // 切换到对应的步骤页面
    ui->contentStackedWidget->setCurrentIndex(index);
}

void MissionPlanner::setRuntimeStage(TaskPlanStage stage, uint simTimestampSec)
{
    m_runtimeStage = stage;
    if (stage == TaskPlanStage::Simulate)
    {
        m_runtimeSimTimestampSec = (simTimestampSec == 0)
                ? static_cast<uint>(QDateTime::currentSecsSinceEpoch())
                : simTimestampSec;
    }
    else
    {
        m_runtimeSimTimestampSec = static_cast<uint>(QDateTime::currentSecsSinceEpoch());
    }

    refreshTaskList();
}

// 任务列表项选择事件处理
void MissionPlanner::onTaskItemSelected(QString taskId)
{
    // 记录当前编辑任务，并回填到步骤1编辑页
    const QString taskUid = m_taskNumberToUid.value(taskId);
    if (taskUid.isEmpty() || !m_taskStore.contains(taskUid))
    {
        return;
    }

    m_currentEditingTaskUid = taskUid;
    const TaskPlanningData &entry = m_taskStore.value(taskUid);
    ui->step1Page->loadTaskForEdit(entry.basicInfo, entry.pointTargets, entry.areaTargets);
    // 自动切换到任务创建页，方便用户直接查看/编辑详情
    ui->contentStackedWidget->setCurrentIndex(0);
    ui->step1Btn->setChecked(true);
}

// 新建任务按钮点击事件
void MissionPlanner::onNewTaskClicked()
{
    // 新建任务：清理当前编辑上下文并重置输入页面
    ui->contentStackedWidget->setCurrentIndex(0);
    ui->step1Btn->setChecked(true);
    m_currentEditingTaskUid.clear();
    ui->step1Page->resetForNewTask();
}

// 删除任务按钮点击事件
void MissionPlanner::onDeleteTaskClicked(QString taskId)
{
    // 从缓存移除对应任务
    const QString taskUid = m_taskNumberToUid.value(taskId);
    if (taskUid.isEmpty())
    {
        return;
    }

    m_taskStore.remove(taskUid);
    m_taskNumberToUid.remove(taskId);
    if (m_currentEditingTaskUid == taskUid)
    {
        // 若删除的是当前正在编辑的任务，重置编辑页避免脏状态
        m_currentEditingTaskUid.clear();
        ui->step1Page->resetForNewTask();
    }

    refreshTaskList();
}

void MissionPlanner::onTaskSaved(const TaskBasicInfo &taskInfo)
{
    // 基础信息兼容入口：保留已有目标列表，仅更新 basicInfo
    TaskPlanningData entry;
    if (!m_currentEditingTaskUid.isEmpty() && m_taskStore.contains(m_currentEditingTaskUid))
    {
        entry = m_taskStore.value(m_currentEditingTaskUid);
    }
    entry.basicInfo = taskInfo;

    if (!m_currentEditingTaskUid.isEmpty() && m_currentEditingTaskUid != taskInfo.taskUid)
    {
        // 支持编辑时修改任务ID：删除旧键，使用新键写入
        m_taskStore.remove(m_currentEditingTaskUid);
    }
    m_taskStore.insert(taskInfo.taskUid, entry);
    m_currentEditingTaskUid = taskInfo.taskUid;
    refreshTaskList();
    onTaskItemSelected(taskInfo.taskId);
}

void MissionPlanner::onTaskSavedDetail(const TaskPlanningData &taskData)
{
    // 标准入口：使用编辑页回传的完整任务数据覆盖缓存
    TaskPlanningData entry = taskData;
    const TaskBasicInfo &taskInfo = entry.basicInfo;

    if (!m_currentEditingTaskUid.isEmpty() && m_currentEditingTaskUid != taskInfo.taskUid)
    {
        m_taskStore.remove(m_currentEditingTaskUid);
    }
    m_taskStore.insert(taskInfo.taskUid, entry);
    m_currentEditingTaskUid = taskInfo.taskUid;

    refreshTaskList();
    onTaskItemSelected(taskInfo.taskId);
}

void MissionPlanner::refreshTaskList()
{
    if (!mTaskListWidget)
    {
        return;
    }

    // 全量重绘任务列表，确保UI与缓存一致
    mTaskListWidget->clearTasks();
    m_taskNumberToUid.clear();

    for (auto it = m_taskStore.cbegin(); it != m_taskStore.cend(); ++it)
    {
        const TaskPlanningData &taskData = it.value();
        const TaskBasicInfo &task = taskData.basicInfo;
        if (!task.taskId.isEmpty())
        {
            m_taskNumberToUid.insert(task.taskId, task.taskUid);
        }
        // 根据当前阶段与仿真时间动态判定任务状态
        const uint statusTime = (m_runtimeStage == TaskPlanStage::Simulate)
                ? m_runtimeSimTimestampSec
                : static_cast<uint>(QDateTime::currentSecsSinceEpoch());
        const TaskStatusType status = resolveTaskStatus(
                m_runtimeStage,
                statusTime,
                task.startTimestampSec,
                task.endTimestampSec);
        const QString statusText = taskStatusTypeToChinese(status);
        // 将时间戳转换为可读时间范围
        const QString timeRange = QString("%1 ~ %2")
                                          .arg(QDateTime::fromSecsSinceEpoch(task.startTimestampSec).toString("yyyy-MM-dd HH:mm"))
                                          .arg(QDateTime::fromSecsSinceEpoch(task.endTimestampSec).toString("yyyy-MM-dd HH:mm"));
        // 目标数量目前使用点目标数量，后续可扩展为点+区域总和
        mTaskListWidget->addTask(task.taskId, task.taskName, statusText, taskData.pointTargets.size(), 6, timeRange);
    }

    // 强制刷新界面
    this->update();
}

void MissionPlanner::generateDemoData()
{
    m_taskStore.clear();
    m_taskNumberToUid.clear();

    const uint nowSec = static_cast<uint>(QDateTime::currentSecsSinceEpoch());

    auto makePointTarget = [](const QString &id, const QString &name, TargetType type,
                              double lat, double lon, const QString &band, const QString &priority)
    {
        PointTargetInfo p;
        p.targetId = id;
        p.name = name;
        p.type = type;
        p.threatLevel = ThreatLevelType::Medium;
        p.latitude = lat;
        p.longitude = lon;
        p.cepMeters = 35.0;
        p.band = band;
        p.emissionPattern = QStringLiteral("间歇");
        p.priority = priority;
        p.requiredPk = 0.85;
        return p;
    };

    auto makeAreaTarget = [](const QString &id, const QString &name, AreaGeometryType areaType,
                             double lat, double lon, double radiusKm)
    {
        AreaTargetInfo a;
        a.targetId = id;
        a.name = name;
        a.areaType = areaType;
        a.centerLatitude = lat;
        a.centerLongitude = lon;
        a.radiusKm = radiusKm;
        a.expectedEmitters = QStringLiteral("预警雷达, 火控雷达");
        a.searchStrategy = SearchStrategyType::SpiralScan;
        a.priority = PriorityLevel::P2;
        return a;
    };

    TaskPlanningData t1;
    t1.basicInfo.taskName = QStringLiteral("东部防空压制任务");
    t1.basicInfo.taskId = QStringLiteral("MSN-2026-0601-A");
    t1.basicInfo.taskType = TaskType::SEAD;
    t1.basicInfo.priority = PriorityLevel::P1;
    t1.basicInfo.status = TaskStatusType::Planning;
    t1.basicInfo.startTimestampSec = nowSec + 3600;
    t1.basicInfo.endTimestampSec = nowSec + 7200;
    t1.basicInfo.intent = QStringLiteral("优先压制东部预警链路，掩护后续打击群突防。");
    t1.pointTargets << makePointTarget("PT-001", "A区预警雷达", TargetType::EarlyWarningRadar, 31.256781, 121.472644, "900~1200MHz", "P1")
                    << makePointTarget("PT-002", "A区火控雷达", TargetType::FireControlRadar, 31.240512, 121.465901, "2800~3200MHz", "P1");
    t1.areaTargets << makeAreaTarget("AT-001", "东部搜索扇区", AreaGeometryType::Circle, 31.248000, 121.470000, 12.5);

    TaskPlanningData t2;
    t2.basicInfo.taskName = QStringLiteral("南部区域侦搜任务");
    t2.basicInfo.taskId = QStringLiteral("MSN-2026-0601-B");
    t2.basicInfo.taskType = TaskType::SEARCH;
    t2.basicInfo.priority = PriorityLevel::P2;
    t2.basicInfo.status = TaskStatusType::Planning;
    t2.basicInfo.startTimestampSec = nowSec + 5400;
    t2.basicInfo.endTimestampSec = nowSec + 10800;
    t2.basicInfo.intent = QStringLiteral("对南部走廊执行持续侦搜，识别潜在辐射源。");
    t2.pointTargets << makePointTarget("PT-101", "南部通信节点", TargetType::CommStation, 30.992311, 121.501274, "450~520MHz", "P2");
    t2.areaTargets << makeAreaTarget("AT-101", "南部走廊区域", AreaGeometryType::Corridor, 30.980000, 121.500000, 18.0)
                   << makeAreaTarget("AT-102", "南部补盲区", AreaGeometryType::Rectangle, 30.945000, 121.455000, 8.0);

    TaskPlanningData t3;
    t3.basicInfo.taskName = QStringLiteral("北部联合作战准备任务");
    t3.basicInfo.taskId = QStringLiteral("MSN-2026-0601-C");
    t3.basicInfo.taskType = TaskType::DEAD;
    t3.basicInfo.priority = PriorityLevel::P3;
    t3.basicInfo.status = TaskStatusType::Planning;
    t3.basicInfo.startTimestampSec = nowSec + 7200;
    t3.basicInfo.endTimestampSec = nowSec + 14400;
    t3.basicInfo.intent = QStringLiteral("清除北部关键雷达节点，建立局部电磁优势。");
    t3.pointTargets << makePointTarget("PT-201", "北部制导雷达", TargetType::GuidanceRadar, 31.402515, 121.356278, "2200~2600MHz", "P2")
                    << makePointTarget("PT-202", "北部干扰设备", TargetType::Jammer, 31.389842, 121.340510, "1500~1900MHz", "P3")
                    << makePointTarget("PT-203", "北部数传节点", TargetType::DataLinkNode, 31.377492, 121.328341, "700~850MHz", "P3");
    t3.areaTargets << makeAreaTarget("AT-201", "北部多边形警戒区", AreaGeometryType::Polygon, 31.390000, 121.340000, 10.0);
    t3.areaTargets.first().vertices << GeoPoint{31.402, 121.326}
                                    << GeoPoint{31.415, 121.348}
                                    << GeoPoint{31.386, 121.362};

    m_taskStore.insert(t1.basicInfo.taskUid, t1);
    m_taskStore.insert(t2.basicInfo.taskUid, t2);
    m_taskStore.insert(t3.basicInfo.taskUid, t3);
}

// 校验任务按钮点击事件
void MissionPlanner::onValidateClicked()
{
}

// 执行任务按钮点击事件
void MissionPlanner::onExecuteClicked()
{
}

// 添加无人机按钮点击事件
void MissionPlanner::onAddUavClicked()
{
}
