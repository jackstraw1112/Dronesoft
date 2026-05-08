// 反辐射无人机任务规划系统 - 主窗口实现文件
// ARUA Mission Planning System - Main Window Implementation

#include "MissionPlanner.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QTimer>
#include "TaskPlanningTypeConvert.h"

namespace
{
constexpr int kTaskDbSchemaVersion = 1;
}

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

    // 初始化任务数据库（建表）并优先从数据库恢复历史数据
    if (!initTaskDatabase())
    {
        QMessageBox::warning(this,
                             QStringLiteral("数据库错误"),
                             QStringLiteral("任务数据库初始化失败，将仅使用内存缓存。"));
    }
    else
    {
        loadTaskStoreFromDatabase();
    }

    // 延时进行首轮列表刷新；数据库为空时自动注入演示数据
    QTimer::singleShot(0, this, [this]()
                       {
        if (m_taskStore.isEmpty())
        {
            generateDemoData();
            for (auto it = m_taskStore.cbegin(); it != m_taskStore.cend(); ++it)
            {
                saveTaskToDatabase(it.value());
            }
        }

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

    if (!deleteTaskFromDatabase(taskUid))
    {
        QMessageBox::warning(this,
                             QStringLiteral("删除失败"),
                             QStringLiteral("数据库删除任务失败，内存数据未变更。"));
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

    const QString oldTaskUid = m_currentEditingTaskUid;
    if (!m_currentEditingTaskUid.isEmpty() && m_currentEditingTaskUid != taskInfo.taskUid)
    {
        // 支持编辑时修改任务ID：删除旧键，使用新键写入
        m_taskStore.remove(m_currentEditingTaskUid);
    }

    if (!saveTaskToDatabase(entry, oldTaskUid))
    {
        QMessageBox::warning(this,
                             QStringLiteral("保存失败"),
                             QStringLiteral("数据库写入任务失败，本次修改未生效。"));
        return;
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

    const QString oldTaskUid = m_currentEditingTaskUid;
    if (!m_currentEditingTaskUid.isEmpty() && m_currentEditingTaskUid != taskInfo.taskUid)
    {
        m_taskStore.remove(m_currentEditingTaskUid);
    }

    if (!saveTaskToDatabase(entry, oldTaskUid))
    {
        QMessageBox::warning(this,
                             QStringLiteral("保存失败"),
                             QStringLiteral("数据库写入任务失败，本次修改未生效。"));
        return;
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

bool MissionPlanner::initTaskDatabase()
{
    const QString connectionName = QStringLiteral("MissionPlannerTaskConnection");
    if (QSqlDatabase::contains(connectionName))
    {
        m_taskDb = QSqlDatabase::database(connectionName);
    }
    else
    {
        m_taskDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    }

    const QString dbDirPath = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    QDir dbDir;
    if (!dbDir.mkpath(dbDirPath))
    {
        qWarning() << "Failed to create db directory:" << dbDirPath;
        return false;
    }

    m_taskDb.setDatabaseName(dbDirPath + QStringLiteral("/task_planning.db"));
    if (!m_taskDb.open())
    {
        qWarning() << "Open task db failed:" << m_taskDb.lastError().text();
        return false;
    }

    QSqlQuery pragmaQuery(m_taskDb);
    if (!pragmaQuery.exec(QStringLiteral("PRAGMA foreign_keys = ON;")))
    {
        qWarning() << "Enable foreign_keys failed:" << pragmaQuery.lastError().text();
        return false;
    }

    if (!createTaskTables())
    {
        return false;
    }
    if (!createTaskIndexes())
    {
        return false;
    }
    return ensureTaskSchemaVersion();
}

bool MissionPlanner::createTaskTables()
{
    if (!m_taskDb.isOpen())
    {
        return false;
    }

    QSqlQuery query(m_taskDb);
    const QString createTaskBasicSql = QStringLiteral(
            "CREATE TABLE IF NOT EXISTS task_basic ("
            "task_uid TEXT PRIMARY KEY,"
            "task_id TEXT,"
            "task_name TEXT,"
            "task_type INTEGER,"
            "priority INTEGER,"
            "status INTEGER,"
            "start_timestamp_sec INTEGER,"
            "end_timestamp_sec INTEGER,"
            "intent TEXT"
            ");");

    const QString createPointTargetSql = QStringLiteral(
            "CREATE TABLE IF NOT EXISTS point_target ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "task_uid TEXT NOT NULL,"
            "order_index INTEGER NOT NULL,"
            "target_id TEXT,"
            "name TEXT,"
            "type INTEGER,"
            "threat_level INTEGER,"
            "latitude REAL,"
            "longitude REAL,"
            "cep_meters REAL,"
            "band TEXT,"
            "emission_pattern TEXT,"
            "priority TEXT,"
            "required_pk REAL,"
            "FOREIGN KEY(task_uid) REFERENCES task_basic(task_uid) ON DELETE CASCADE"
            ");");

    const QString createAreaTargetSql = QStringLiteral(
            "CREATE TABLE IF NOT EXISTS area_target ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "task_uid TEXT NOT NULL,"
            "order_index INTEGER NOT NULL,"
            "target_id TEXT,"
            "name TEXT,"
            "area_type INTEGER,"
            "center_latitude REAL,"
            "center_longitude REAL,"
            "radius_km REAL,"
            "vertices_json TEXT,"
            "expected_emitters TEXT,"
            "search_strategy INTEGER,"
            "priority INTEGER,"
            "FOREIGN KEY(task_uid) REFERENCES task_basic(task_uid) ON DELETE CASCADE"
            ");");

    if (!query.exec(createTaskBasicSql))
    {
        qWarning() << "Create task_basic failed:" << query.lastError().text();
        return false;
    }
    if (!query.exec(createPointTargetSql))
    {
        qWarning() << "Create point_target failed:" << query.lastError().text();
        return false;
    }
    if (!query.exec(createAreaTargetSql))
    {
        qWarning() << "Create area_target failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool MissionPlanner::createTaskIndexes()
{
    if (!m_taskDb.isOpen())
    {
        return false;
    }

    QSqlQuery query(m_taskDb);
    const QStringList indexSqlList = {
        QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS idx_task_basic_task_id ON task_basic(task_id);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_point_target_task_uid_order ON point_target(task_uid, order_index);"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_area_target_task_uid_order ON area_target(task_uid, order_index);")
    };

    for (const QString &sql : indexSqlList)
    {
        if (!query.exec(sql))
        {
            qWarning() << "Create index failed:" << query.lastError().text() << "sql:" << sql;
            return false;
        }
    }
    return true;
}

bool MissionPlanner::ensureTaskSchemaVersion()
{
    if (!m_taskDb.isOpen())
    {
        return false;
    }

    QSqlQuery query(m_taskDb);
    if (!query.exec(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS db_meta ("
                "meta_key TEXT PRIMARY KEY,"
                "meta_value TEXT"
                ");")))
    {
        qWarning() << "Create db_meta failed:" << query.lastError().text();
        return false;
    }

    int currentVersion = 0;
    query.prepare(QStringLiteral("SELECT meta_value FROM db_meta WHERE meta_key = ?;"));
    query.addBindValue(QStringLiteral("schema_version"));
    if (!query.exec())
    {
        qWarning() << "Read schema_version failed:" << query.lastError().text();
        return false;
    }
    if (query.next())
    {
        currentVersion = query.value(0).toInt();
    }

    if (currentVersion > kTaskDbSchemaVersion)
    {
        qWarning() << "Database schema is newer than application supports. current="
                   << currentVersion << " supported=" << kTaskDbSchemaVersion;
        return false;
    }

    if (currentVersion < kTaskDbSchemaVersion)
    {
        // 当前版本为V1，表结构已由 createTaskTables/createTaskIndexes 保证存在，
        // 此处只写入版本号。后续升级可在这里按版本号增量迁移。
        query.prepare(QStringLiteral(
                "INSERT OR REPLACE INTO db_meta(meta_key, meta_value) VALUES(?, ?);"));
        query.addBindValue(QStringLiteral("schema_version"));
        query.addBindValue(QString::number(kTaskDbSchemaVersion));
        if (!query.exec())
        {
            qWarning() << "Write schema_version failed:" << query.lastError().text();
            return false;
        }
    }

    return true;
}

bool MissionPlanner::loadTaskStoreFromDatabase()
{
    if (!m_taskDb.isOpen())
    {
        return false;
    }

    m_taskStore.clear();
    m_taskNumberToUid.clear();

    QSqlQuery taskQuery(m_taskDb);
    if (!taskQuery.exec(QStringLiteral(
                "SELECT task_uid, task_id, task_name, task_type, priority, status, "
                "start_timestamp_sec, end_timestamp_sec, intent "
                "FROM task_basic ORDER BY rowid ASC;")))
    {
        qWarning() << "Load task_basic failed:" << taskQuery.lastError().text();
        return false;
    }

    while (taskQuery.next())
    {
        TaskPlanningData taskData;
        taskData.basicInfo.taskUid = taskQuery.value(0).toString();
        taskData.basicInfo.taskId = taskQuery.value(1).toString();
        taskData.basicInfo.taskName = taskQuery.value(2).toString();
        taskData.basicInfo.taskType = static_cast<TaskType>(taskQuery.value(3).toInt());
        taskData.basicInfo.priority = static_cast<PriorityLevel>(taskQuery.value(4).toInt());
        taskData.basicInfo.status = static_cast<TaskStatusType>(taskQuery.value(5).toInt());
        taskData.basicInfo.startTimestampSec = taskQuery.value(6).toUInt();
        taskData.basicInfo.endTimestampSec = taskQuery.value(7).toUInt();
        taskData.basicInfo.intent = taskQuery.value(8).toString();

        QSqlQuery pointQuery(m_taskDb);
        pointQuery.prepare(QStringLiteral(
                "SELECT target_id, name, type, threat_level, latitude, longitude, "
                "cep_meters, band, emission_pattern, priority, required_pk "
                "FROM point_target WHERE task_uid = ? ORDER BY order_index ASC;"));
        pointQuery.addBindValue(taskData.basicInfo.taskUid);
        if (!pointQuery.exec())
        {
            qWarning() << "Load point_target failed:" << pointQuery.lastError().text();
            return false;
        }
        while (pointQuery.next())
        {
            PointTargetInfo p;
            p.targetId = pointQuery.value(0).toString();
            p.name = pointQuery.value(1).toString();
            p.type = static_cast<TargetType>(pointQuery.value(2).toInt());
            p.threatLevel = static_cast<ThreatLevelType>(pointQuery.value(3).toInt());
            p.latitude = pointQuery.value(4).toDouble();
            p.longitude = pointQuery.value(5).toDouble();
            p.cepMeters = pointQuery.value(6).toDouble();
            p.band = pointQuery.value(7).toString();
            p.emissionPattern = pointQuery.value(8).toString();
            p.priority = pointQuery.value(9).toString();
            p.requiredPk = pointQuery.value(10).toDouble();
            taskData.pointTargets.append(p);
        }

        QSqlQuery areaQuery(m_taskDb);
        areaQuery.prepare(QStringLiteral(
                "SELECT target_id, name, area_type, center_latitude, center_longitude, "
                "radius_km, vertices_json, expected_emitters, search_strategy, priority "
                "FROM area_target WHERE task_uid = ? ORDER BY order_index ASC;"));
        areaQuery.addBindValue(taskData.basicInfo.taskUid);
        if (!areaQuery.exec())
        {
            qWarning() << "Load area_target failed:" << areaQuery.lastError().text();
            return false;
        }
        while (areaQuery.next())
        {
            AreaTargetInfo a;
            a.targetId = areaQuery.value(0).toString();
            a.name = areaQuery.value(1).toString();
            a.areaType = static_cast<AreaGeometryType>(areaQuery.value(2).toInt());
            a.centerLatitude = areaQuery.value(3).toDouble();
            a.centerLongitude = areaQuery.value(4).toDouble();
            a.radiusKm = areaQuery.value(5).toDouble();
            a.expectedEmitters = areaQuery.value(7).toString();
            a.searchStrategy = static_cast<SearchStrategyType>(areaQuery.value(8).toInt());
            a.priority = static_cast<PriorityLevel>(areaQuery.value(9).toInt());

            const QByteArray verticesJson = areaQuery.value(6).toByteArray();
            const QJsonDocument doc = QJsonDocument::fromJson(verticesJson);
            if (doc.isArray())
            {
                const QJsonArray points = doc.array();
                for (const QJsonValue &pointValue : points)
                {
                    if (!pointValue.isObject())
                    {
                        continue;
                    }
                    const QJsonObject pointObj = pointValue.toObject();
                    GeoPoint point;
                    point.latitude = pointObj.value(QStringLiteral("latitude")).toDouble();
                    point.longitude = pointObj.value(QStringLiteral("longitude")).toDouble();
                    a.vertices.append(point);
                }
            }

            taskData.areaTargets.append(a);
        }

        m_taskStore.insert(taskData.basicInfo.taskUid, taskData);
    }

    return true;
}

bool MissionPlanner::saveTaskToDatabase(const TaskPlanningData &taskData, const QString &oldTaskUid)
{
    if (!m_taskDb.isOpen())
    {
        return false;
    }

    if (!m_taskDb.transaction())
    {
        qWarning() << "DB begin transaction failed:" << m_taskDb.lastError().text();
        return false;
    }

    QSqlQuery query(m_taskDb);
    auto rollbackAndFail = [&]()
    {
        m_taskDb.rollback();
        return false;
    };

    if (!oldTaskUid.isEmpty() && oldTaskUid != taskData.basicInfo.taskUid)
    {
        query.prepare(QStringLiteral("DELETE FROM task_basic WHERE task_uid = ?;"));
        query.addBindValue(oldTaskUid);
        if (!query.exec())
        {
            qWarning() << "Delete old task_basic failed:" << query.lastError().text();
            return rollbackAndFail();
        }
    }

    query.prepare(QStringLiteral("DELETE FROM task_basic WHERE task_uid = ?;"));
    query.addBindValue(taskData.basicInfo.taskUid);
    if (!query.exec())
    {
        qWarning() << "Delete existing task_basic failed:" << query.lastError().text();
        return rollbackAndFail();
    }

    query.prepare(QStringLiteral(
            "INSERT INTO task_basic(task_uid, task_id, task_name, task_type, priority, status, "
            "start_timestamp_sec, end_timestamp_sec, intent) "
            "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);"));
    query.addBindValue(taskData.basicInfo.taskUid);
    query.addBindValue(taskData.basicInfo.taskId);
    query.addBindValue(taskData.basicInfo.taskName);
    query.addBindValue(static_cast<int>(taskData.basicInfo.taskType));
    query.addBindValue(static_cast<int>(taskData.basicInfo.priority));
    query.addBindValue(static_cast<int>(taskData.basicInfo.status));
    query.addBindValue(taskData.basicInfo.startTimestampSec);
    query.addBindValue(taskData.basicInfo.endTimestampSec);
    query.addBindValue(taskData.basicInfo.intent);
    if (!query.exec())
    {
        qWarning() << "Insert task_basic failed:" << query.lastError().text();
        return rollbackAndFail();
    }

    int orderIndex = 0;
    for (const PointTargetInfo &point : taskData.pointTargets)
    {
        query.prepare(QStringLiteral(
                "INSERT INTO point_target(task_uid, order_index, target_id, name, type, threat_level, "
                "latitude, longitude, cep_meters, band, emission_pattern, priority, required_pk) "
                "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"));
        query.addBindValue(taskData.basicInfo.taskUid);
        query.addBindValue(orderIndex++);
        query.addBindValue(point.targetId);
        query.addBindValue(point.name);
        query.addBindValue(static_cast<int>(point.type));
        query.addBindValue(static_cast<int>(point.threatLevel));
        query.addBindValue(point.latitude);
        query.addBindValue(point.longitude);
        query.addBindValue(point.cepMeters);
        query.addBindValue(point.band);
        query.addBindValue(point.emissionPattern);
        query.addBindValue(point.priority);
        query.addBindValue(point.requiredPk);
        if (!query.exec())
        {
            qWarning() << "Insert point_target failed:" << query.lastError().text();
            return rollbackAndFail();
        }
    }

    orderIndex = 0;
    for (const AreaTargetInfo &area : taskData.areaTargets)
    {
        QJsonArray verticesArray;
        for (const GeoPoint &point : area.vertices)
        {
            QJsonObject pointObj;
            pointObj.insert(QStringLiteral("latitude"), point.latitude);
            pointObj.insert(QStringLiteral("longitude"), point.longitude);
            verticesArray.append(pointObj);
        }
        const QString verticesJson = QString::fromUtf8(QJsonDocument(verticesArray).toJson(QJsonDocument::Compact));

        query.prepare(QStringLiteral(
                "INSERT INTO area_target(task_uid, order_index, target_id, name, area_type, center_latitude, "
                "center_longitude, radius_km, vertices_json, expected_emitters, search_strategy, priority) "
                "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"));
        query.addBindValue(taskData.basicInfo.taskUid);
        query.addBindValue(orderIndex++);
        query.addBindValue(area.targetId);
        query.addBindValue(area.name);
        query.addBindValue(static_cast<int>(area.areaType));
        query.addBindValue(area.centerLatitude);
        query.addBindValue(area.centerLongitude);
        query.addBindValue(area.radiusKm);
        query.addBindValue(verticesJson);
        query.addBindValue(area.expectedEmitters);
        query.addBindValue(static_cast<int>(area.searchStrategy));
        query.addBindValue(static_cast<int>(area.priority));
        if (!query.exec())
        {
            qWarning() << "Insert area_target failed:" << query.lastError().text();
            return rollbackAndFail();
        }
    }

    if (!m_taskDb.commit())
    {
        qWarning() << "DB commit failed:" << m_taskDb.lastError().text();
        return false;
    }
    return true;
}

bool MissionPlanner::deleteTaskFromDatabase(const QString &taskUid)
{
    if (!m_taskDb.isOpen())
    {
        return false;
    }

    QSqlQuery query(m_taskDb);
    query.prepare(QStringLiteral("DELETE FROM task_basic WHERE task_uid = ?;"));
    query.addBindValue(taskUid);
    if (!query.exec())
    {
        qWarning() << "Delete task_basic failed:" << query.lastError().text();
        return false;
    }
    return true;
}
