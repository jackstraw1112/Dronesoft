// 反辐射无人机任务规划系统 - 主窗口头文件
// ARUA Mission Planning System - Main Window Header

#ifndef MISSIONPLANNER_H
#define MISSIONPLANNER_H

#include <QMainWindow>
#include <QButtonGroup>
#include "TaskListWidget.h"
#include "RightSidePanel.h"
#include "WindowStyle.h"
#include "ui_MissionPlanner.h"

// 任务规划主窗口类，继承自QMainWindow
class MissionPlanner : public QMainWindow
{
    Q_OBJECT

public:
    // 构造函数，parent为父窗口指针，默认为nullptr
    explicit MissionPlanner(QWidget *parent = nullptr);

    // 析构函数，释放资源
    ~MissionPlanner();

    // 静态单例
    static MissionPlanner *m_pInstance;
    static MissionPlanner *GetInstance(QWidget *parent = nullptr);

    QButtonGroup *stepButtonGroup;   // 步骤导航按钮组（5个步骤按钮）

    //任务列表
    TaskListWidget* mTaskListWidget=nullptr;

    //资源
    RightSidePanel* mRightSidePanel=nullptr;

    // 初始化UI界面设置
    void initUI();
    // 设置信号槽连接
    void setupConnections();
    // 初始化基本信息表单（任务名称、类型、优先级等）
    void initBasicInfoForm();
    // 初始化目标信息表格（点目标表、面目标详情）
    void initTargetTables();
    // 初始化兵力计算面板（无人机数量、编队间距、毁伤概率等）
    void initForceCalcPanel();

private slots:
    // 步骤导航切换事件处理（0-4对应5个步骤）
    void onStepChanged(int stepIndex);
    // 任务列表项选择事件处理
    void onTaskItemSelected(int taskIndex);
    // 新建任务按钮点击事件
    void onNewTaskClicked();
    // 导入任务按钮点击事件
    void onImportTasksClicked();
    // 保存任务按钮点击事件
    void onSaveTaskClicked();
    // 校验任务按钮点击事件
    void onValidateClicked();
    // 执行任务按钮点击事件
    void onExecuteClicked();
    // 添加无人机按钮点击事件
    void onAddUavClicked();

private:
    Ui_MissionPlanner *ui;           // UI界面对象指针
};

#endif // MISSIONPLANNER_H
