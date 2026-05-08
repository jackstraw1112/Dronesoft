// 反辐射无人机任务规划系统 - 主窗口头文件
// ARUA Mission Planning System - Main Window Header

#ifndef MISSIONPLANNER_H
#define MISSIONPLANNER_H

#include <QButtonGroup>
#include <QMainWindow>
#include <QMap>
#include <QSqlDatabase>
#include "01-TaskPlan/RZTaskListWidget.h"
#include "RightSidePanel.h"
#include "TaskPlanningData.h"
#include "WindowStyle.h"
#include "ui_MissionPlanner.h"

// 任务规划主窗口类，继承自QMainWindow
class MissionPlanner : public QMainWindow
{
    Q_OBJECT

public:
    // 单例访问
    static MissionPlanner *GetInstance(QWidget *parent = nullptr);
    // 设置运行阶段与仿真时间（仿真阶段下用于判定任务执行状态）
    void setRuntimeStage(TaskPlanStage stage, uint simTimestampSec = 0);

<<<<<<< HEAD
    QButtonGroup *stepButtonGroup;   // 步骤导航按钮组（5个步骤按钮）

    //任务列表
    TaskListWidget* mTaskListWidget=nullptr;

    //资源
    RightSidePanel* mRightSidePanel=nullptr;

    // 初始化UI界面设置
    void initUI();
    // 设置信号槽连接
    void setupConnections();
    // 应用科技风格样式（与ForceRequirementPanel一致）
    void applyTechStyle();
    // 初始化基本信息表单（任务名称、类型、优先级等）
    void initBasicInfoForm();
    // 初始化目标信息表格（点目标表、面目标详情）
    void initTargetTables();
    // 初始化兵力计算面板（无人机数量、编队间距、毁伤概率等）
    void initForceCalcPanel();
=======
private:
    explicit MissionPlanner(QWidget *parent = nullptr);
    ~MissionPlanner();
    MissionPlanner(const MissionPlanner &) = delete;
    MissionPlanner &operator=(const MissionPlanner &) = delete;
    MissionPlanner(MissionPlanner &&) = delete;
    MissionPlanner &operator=(MissionPlanner &&) = delete;
>>>>>>> f18b2aacc4dd0c690feeb24333d8b981fd8b0e69

private slots:
    // 步骤导航切换事件处理
    void onStepChanged(int index);
    // 新建任务按钮点击事件
    void onNewTaskClicked();
    // 删除任务按钮点击事件
    void onDeleteTaskClicked(QString taskId);
    // 任务列表项选择事件处理
    void onTaskItemSelected(QString taskId);
    // 任务编辑页保存回调
    void onTaskSaved(const TaskBasicInfo &taskInfo);
    // 校验任务按钮点击事件
    void onValidateClicked();
    // 执行任务按钮点击事件
    void onExecuteClicked();
    // 添加无人机按钮点击事件
    void onAddUavClicked();

private:
    // 初始化参数
    void initParams();
    // 初始化对象
    void initObject();
    // 关联信号与槽函数
    void initConnect();

    // 将任务缓存重新刷新到左侧列表
    void refreshTaskList();
    // 生成临时演示数据
    void generateDemoData();
    // 完整任务保存回调
    void onTaskSavedDetail(const TaskPlanningData &taskData);
    // 初始化数据库连接并创建数据表
    bool initTaskDatabase();
    // 创建任务相关数据表（主表 + 子表）
    bool createTaskTables();
    // 创建常用查询索引
    bool createTaskIndexes();
    // 确保数据库结构版本正确（含升级迁移）
    bool ensureTaskSchemaVersion();
    // 从数据库加载任务缓存
    bool loadTaskStoreFromDatabase();
    // 保存单条任务到数据库（支持编辑时 oldTaskUid -> newTaskUid）
    bool saveTaskToDatabase(const TaskPlanningData &taskData, const QString &oldTaskUid = QString());
    // 根据任务UID删除数据库中的单条任务
    bool deleteTaskFromDatabase(const QString &taskUid);

private:
    Ui_MissionPlanner *ui;

    // 任务缓存（key=taskUid，系统自动生成）
    QMap<QString, TaskPlanningData> m_taskStore;

    // 任务编号到系统任务ID映射（用于列表选中/删除转换）
    QMap<QString, QString> m_taskNumberToUid;

    // 当前编辑中的系统任务ID
    QString m_currentEditingTaskUid;
    // 当前运行阶段（默认方案阶段）
    TaskPlanStage m_runtimeStage = TaskPlanStage::Scheme;
    // 当前仿真时间戳（秒）
    uint m_runtimeSimTimestampSec = 0;

    // 步骤导航按钮组（5个步骤按钮）
    QButtonGroup *stepButtonGroup;
    // 任务数据库连接
    QSqlDatabase m_taskDb;

    // 任务列表
    RZTaskListWidget *mTaskListWidget = nullptr;

    // 资源
    RightSidePanel *mRightSidePanel = nullptr;
};

#endif // MISSIONPLANNER_H
