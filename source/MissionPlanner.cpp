// 反辐射无人机任务规划系统 - 主窗口实现文件
// ARUA Mission Planning System - Main Window Implementation

#include "MissionPlanner.h"
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>

// 单例对象实现
MissionPlanner *MissionPlanner::m_pInstance = nullptr;


MissionPlanner *MissionPlanner::GetInstance(QWidget *parent)
{
    if (m_pInstance == nullptr)
    {
        m_pInstance = new MissionPlanner(parent);
    }
    return m_pInstance;
}

// 构造函数：初始化UI界面、建立信号槽连接、初始化各面板数据
MissionPlanner::MissionPlanner(QWidget *parent)
    : QMainWindow(parent), ui(new Ui_MissionPlanner)
{
    ui->setupUi(this);          // 加载UI文件并设置界面
    initUI();                   // 初始化UI组件（按钮组、时间显示等）
    setupConnections();         // 建立信号槽连接
}

// 析构函数：释放UI资源
MissionPlanner::~MissionPlanner()
{
    delete ui;
}

// 初始化UI界面设置
void MissionPlanner::initUI()
{
#if WIN32
    //setWindowFlags(Qt::FramelessWindowHint | Qt::Window);	//窗口头部隐藏
#else
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);	//窗口头部隐藏
#endif
    //setAttribute(Qt::WA_TranslucentBackground, false);		//窗口透明
    //setWindowOpacity(0.85);
    //setAutoFillBackground(true);

    //WindowStyle* _pWindowStyle = new WindowStyle();
    //_pWindowStyle->activateOn(ui->widget_Title);

    // 创建步骤导航按钮组，包含5个步骤按钮（任务创建、兵力需求计算、协同任务分配、航路规划、参数装订）
    stepButtonGroup = new QButtonGroup(this);
    stepButtonGroup->addButton(ui->step1Btn, 0);   // 步骤1：任务创建
    stepButtonGroup->addButton(ui->step2Btn, 1);   // 步骤2：兵力需求计算
    stepButtonGroup->addButton(ui->step3Btn, 2);   // 步骤3：协同任务分配
    stepButtonGroup->addButton(ui->step4Btn, 3);   // 步骤4：航路规划
    stepButtonGroup->addButton(ui->step5Btn, 4);   // 步骤5：参数装订
    stepButtonGroup->setExclusive(true);           // 设置按钮互斥（单选）

    // 默认显示第一个步骤页面
    ui->contentStackedWidget->setCurrentIndex(0);
    ui->step1Btn->setChecked(true);

    //添加布局
    //QGridLayout* mTaskListWidgetLayout = new QGridLayout(ui->widget_Task);
    //mTaskListWidget=new TaskListWidget();
    //mTaskListWidgetLayout->addWidget(mTaskListWidget);
    //mTaskListWidgetLayout->setContentsMargins(0,0,0,0);

    //QGridLayout* mRightSidePanelWidgetLayout = new QGridLayout(ui->widget_Source);
    //mRightSidePanel=new RightSidePanel();
    //mRightSidePanelWidgetLayout->addWidget(mRightSidePanel);
    //mRightSidePanelWidgetLayout->setContentsMargins(0,0,0,0);
}

// 设置信号槽连接：将UI控件的信号与对应的槽函数绑定
void MissionPlanner::setupConnections()
{
    // 步骤导航按钮切换信号 -> onStepChanged槽函数
    connect(stepButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onStepChanged(int)));
}

// 步骤导航切换事件处理（0-4对应5个步骤）
void MissionPlanner::onStepChanged(int stepIndex)
{
    // 切换到对应的步骤页面
    ui->contentStackedWidget->setCurrentIndex(stepIndex);

    // 更新按钮样式（如果有的话可以通过QSS实现）
    qDebug() << "Step changed to:" << stepIndex;
}

// 任务列表项选择事件处理
void MissionPlanner::onTaskItemSelected(int taskIndex)
{
    qDebug() << "Task selected:" << taskIndex;
}

// 新建任务按钮点击事件
void MissionPlanner::onNewTaskClicked()
{

}

// 导入任务按钮点击事件
void MissionPlanner::onImportTasksClicked()
{

}

// 保存任务按钮点击事件
void MissionPlanner::onSaveTaskClicked()
{

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

// 初始化基本信息表单：填充下拉框选项
void MissionPlanner::initBasicInfoForm()
{
    // 如果UI中有这些控件，则填充选项
    // 注意：当前UI文件中不包含这些控件，暂不处理
}

// 初始化目标信息表格（点目标表、面目标详情）
void MissionPlanner::initTargetTables()
{
    // 如果UI中有这些控件，则初始化表格
    // 注意：当前UI文件中不包含这些控件，暂不处理
}

// 初始化兵力计算面板（无人机数量、编队间距、毁伤概率等）
void MissionPlanner::initForceCalcPanel()
{
    // 如果UI中有这些控件，则初始化
    // 注意：当前UI文件中不包含这些控件，暂不处理
}
