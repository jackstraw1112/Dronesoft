//
// Created by Administrator on 2026/5/6.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_TASKALLOCATIONPANEL_H
#define RZSIM_ANTI_RADIATION_UAV_TASKALLOCATIONPANEL_H

#include <QFrame>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QRadioButton>
#include <QProgressBar>
#include <QTimer>
#include <QScrollArea>
#include <QMap>
#include "TaskPlanningData.h"

class QVBoxLayout;
class QGridLayout;


QT_BEGIN_NAMESPACE
namespace Ui { class TaskAllocationPanel; }
QT_END_NAMESPACE


// ForceTargetData / AltPlanData / UavSpec 定义已统一移至 TaskPlanningData.h


class TaskAllocationPanel : public QFrame {
Q_OBJECT

public:
    explicit TaskAllocationPanel(QWidget *parent = nullptr);
    ~TaskAllocationPanel() override;

    // ═══════════════════════════════════════════
    // 兵力数据注入（由 MissionPlanner 在步骤切换时调用）
    // ═══════════════════════════════════════════
    // 设置从兵力需求面板传入的目标列表和计算结果
    // @param targets       目标列表（id / name / type / aircraftCount）
    // @param ptResults     点目标计算结果映射表
    // @param arResults     区域目标计算结果映射表
    void setForceData(const QList<ForceTargetData> &targets,
                      const QMap<QString, PtCalcData> &ptResults = QMap<QString, PtCalcData>(),
                      const QMap<QString, ArCalcData> &arResults = QMap<QString, ArCalcData>());

    // 获取当前兵力目标数据（供 MissionPlanner 在步骤切换时传递给 RoutePlanning）
    const QList<ForceTargetData> &forceTargets() const { return m_forceTargets; }

    // ═══════════════════════════════════════════
    // 求解算法 API
    // ═══════════════════════════════════════════

    // 添加一种求解算法卡片
    // @param name       算法显示名称（如 "匈牙利算法 · Hungarian"）
    // @param desc       算法描述（如 "指派问题最优解 · 适用于无人机数 ≈ 目标数"）
    // @param badge      徽章文本（如 "FAST" / "SLOW" / "DIST"）
    // @param badgeColor 徽章背景色（如 "#00e676"）
    // @return           算法索引 id
    int addAlgorithm(const QString &name, const QString &desc,
                     const QString &badge, const QString &badgeColor);

    // 移除指定索引的算法卡片
    void removeAlgorithm(int index);

    // 清空所有算法卡片
    void clearAlgorithms();

    // 设置当前选中的算法（通过索引，从 0 开始）
    void setCurrentAlgorithm(int index);

    // 获取当前选中算法的索引，-1 表示无选中
    int currentAlgorithm() const;

    // 获取算法总数
    int algorithmCount() const;

    // ═══════════════════════════════════════════
    // 协同硬约束 API
    // ═══════════════════════════════════════════

    // 添加一条协同硬约束
    // @param name     约束名称（如 "导引头频段匹配"）
    // @param desc     约束描述
    // @param checked  默认是否勾选
    // @return         约束索引 id
    int addConstraint(const QString &name, const QString &desc, bool checked = true);

    // 移除指定索引的约束行
    void removeConstraint(int index);

    // 清空所有约束
    void clearConstraints();

    // 设置指定索引约束的勾选状态
    void setConstraintChecked(int index, bool checked);

    // 查询指定索引约束是否勾选
    bool isConstraintChecked(int index) const;

    // 获取约束总数
    int constraintCount() const;

    // ═══════════════════════════════════════════
    // 求解指标 API
    // ═══════════════════════════════════════════

    // 添加一项求解指标卡片
    // @param label  指标标签（如 "目标函数值"）
    // @param value  指标数值（如 "0.943"）
    // @param unit   单位（如 "%" / "ms"）
    // @param tag    标记文本（如 "较初始 +47%"）
    // @param color  数值显示颜色
    // @return       指标索引 id
    int addMetric(const QString &label, const QString &value,
                  const QString &unit = QString(), const QString &tag = QString(),
                  const QString &color = "#00e5ff");

    // 更新指定索引的指标数值和标记
    // @param index 指标索引
    // @param value 新数值
    // @param tag   新标记文本（空字符串表示不更新）
    // @param color 新颜色（空字符串表示不更新）
    void updateMetric(int index, const QString &value,
                      const QString &tag = QString(), const QString &color = QString());

    // 移除指定索引的指标卡片
    void removeMetric(int index);

    // 清空所有指标卡片
    void clearMetrics();

    // 获取指标总数
    int metricCount() const;

    // ═══════════════════════════════════════════
    // 候选方案 API
    // ═══════════════════════════════════════════

    // 添加一条候选方案行
    // @param name      方案名称（如 "方案 1 · 当前"）
    // @param fval      目标函数值
    // @param sorties   动用架次
    // @param risk      风险等级
    // @param isCurrent 是否为当前选中方案
    // @return          方案索引 id
    int addAltPlan(const QString &name, const QString &fval,
                   const QString &sorties, const QString &risk,
                   bool isCurrent = false);

    // 移除指定索引的候选方案行
    void removeAltPlan(int index);

    // 清空所有候选方案
    void clearAltPlans();

    // 获取候选方案总数
    int altPlanCount() const;

    // ═══════════════════════════════════════════
    // 编队分配方案 API
    // ═══════════════════════════════════════════

    // 无人机规格结构体，描述编队中每架 UAV 的标识、角色和元信息
    struct UavSpec {
        QString id;      // 无人机编号（如 "UAV-A01"）
        QString role;    // 角色描述（如 "主攻 1" / "备份"）
        QString meta;    // 元信息（如 "I-band · 62km"）
    };

    // 添加一个目标编队分配组
    // @param target    目标编号（如 "PT-01"）
    // @param name      目标名称（如 "东郊制导雷达"）
    // @param priority  优先级（如 "P1" / "P2"）
    // @param tot       同时到达时刻（如 "TOT 14:42:18"）
    // @param uavs      无人机编队列表
    // @param coordDesc 协同方式描述
    // @return          分配组索引 id
    int addAllocGroup(const QString &target, const QString &name,
                      const QString &priority, const QString &tot,
                      const QList<UavSpec> &uavs, const QString &coordDesc);

    // 移除指定索引的编队分配组
    void removeAllocGroup(int index);

    // 清空所有编队分配组
    void clearAllocGroups();

    // 获取编队分配组总数
    int allocGroupCount() const;

private slots:
    // 求解按钮点击：按当前选中算法重新生成分配方案
    void onSolveClicked();

    // 求解算法切换时更新目标函数权重
    void onAlgChanged(int id);

    // 候选方案选用：切换分配方案并重新生成编队分组
    void onAltPlanClicked(int index);

private:
    // 应用深色科技风全局样式
    void applyTechStyle();

    // 初始化各功能组（调用上述 API 填入默认数据）
    void setupAlgorithmGroup();
    void setupConstraintGroup();
    void setupMetricsGroup();
    void setupAllocationResult();

    // 修正 .ui 中目标函数权重控件的文本和样式
    void setupWeightControls();

    // 根据当前选中算法和兵力数据生成分配方案
    void generateAllocationResult();

    // 应用指定索引的候选方案（重新生成编队分配组并更新指标）
    void applyAltPlan(int index);

    // 根据算法索引启动权重平滑动画
    void animateWeights();

    // ═══════════════════════════════════════════
    // 控件工厂（创建单个控件，不管理生命周期）
    // ═══════════════════════════════════════════

    // 创建一个算法选择卡片（含 radio + 名称 + 描述 + 徽章）
    QFrame *createAlgCard(const QString &name, const QString &desc,
                          const QString &badge, const QString &badgeColor, int id);

    // 创建一个约束行（含 checkbox + 名称 + 描述）
    QFrame *createConstraintRow(const QString &name, const QString &desc,
                                bool checked, int id);

    // 创建一个指标卡片（含标签 + 大数值 + 单位 + 标记）
    QFrame *createMetricCard(const QString &label, const QString &value,
                             const QString &unit, const QString &tag,
                             const QString &color);

    // 创建一个候选方案行（含名称 + F 值 + 架次 + 风险 + 状态）
    QFrame *createAltPlanRow(const QString &name, const QString &fval,
                             const QString &sorties, const QString &risk,
                             bool isCurrent);

    // 创建一个编队分配组框架（含目标头 + UAV 芯片行 + 协同方式条）
    QFrame *createAllocGroupFrame(const QString &target, const QString &name,
                                  const QString &priority, const QString &tot,
                                  const QList<UavSpec> &uavs,
                                  const QString &coordDesc);

    Ui::TaskAllocationPanel *ui;

    // ─── 算法成员 ───
    QButtonGroup *m_algGroup;       // 算法 radio 互斥组
    QList<QFrame*> m_algCards;      // 所有算法卡片
    QList<QRadioButton*> m_algRadios; // 所有算法 radio
    QTimer *m_weightTimer;            // 权重动画定时器（60fps）
    int m_weightElapsed;              // 动画已流逝毫秒数
    int m_weightFrom[4];              // 动画起始权重值
    int m_weightTo[4];                // 动画目标权重值
    QProgressBar *m_weightBars[4];    // 权重进度条缓存（避免运行时 findChild）
    QLabel *m_weightLabels[4];        // 权重值标签缓存（避免运行时 findChild）

    // ─── 约束成员 ───
    QList<QFrame*> m_constraintRows;    // 所有约束行容器
    QList<QCheckBox*> m_constraintChecks; // 所有约束 checkbox

    // ─── 指标成员 ───
    QGridLayout *m_metricGrid;      // 指标卡片网格布局
    QList<QFrame*> m_metricCards;   // 所有指标卡片
    QList<QLabel*> m_metricValues;  // 指标数值标签（供 updateMetric 使用）
    QList<QLabel*> m_metricUnits;   // 指标单位标签（供 updateMetric 使用）
    QList<QLabel*> m_metricTags;    // 指标标记标签（供 updateMetric 使用）

    // ─── 候选方案成员 ───
    QVBoxLayout *m_altPlanLayout;   // 候选方案容器布局
    QList<QFrame*> m_altPlanRows;   // 所有候选方案行
    QList<QPushButton*> m_altPlanBtns;  // 所有候选方案的"选用"按钮
    QList<AltPlanData> m_altPlanDataList; // 候选方案数据列表
    int m_currentAltPlan = 0;       // 当前选中方案索引

    // ─── 编队分配成员 ───
    QVBoxLayout *m_allocLayout;         // widget_2 外布局（标题 + 滚动区 + 弹簧）
    QScrollArea *m_allocScrollArea;     // 编队分组滚动区
    QVBoxLayout *m_allocScrollLayout;   // 滚动区内布局（存放所有分配组卡片）
    QList<QFrame*> m_allocFrames;       // 所有编队分配组

    // ─── 兵力数据成员（来自 ForceRequirementPanel） ───
    QList<ForceTargetData> m_forceTargets;           // 目标列表
    QMap<QString, PtCalcData> m_forcePtResults;     // 点目标计算结果
    QMap<QString, ArCalcData> m_forceArResults;     // 区域目标计算结果
    QList<ForceTargetData> m_lastTargets;            // 最近一次求解使用的目标数据（用于切换候选方案）
};


#endif //RZSIM_ANTI_RADIATION_UAV_TASKALLOCATIONPANEL_H
