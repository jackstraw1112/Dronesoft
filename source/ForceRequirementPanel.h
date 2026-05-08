//
// Created by Administrator on 2026/5/6.
//
// 兵力需求计算面板 - 头文件

#ifndef RZSIM_ANTI_RADIATION_UAV_FORCEREQUIREMENTPANEL_H
#define RZSIM_ANTI_RADIATION_UAV_FORCEREQUIREMENTPANEL_H

#include <QFrame>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QGroupBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QMap>
#include <QGridLayout>
#include <QScrollArea>
#include "StructData.h"


QT_BEGIN_NAMESPACE
namespace Ui { class ForceRequirementPanel; }
QT_END_NAMESPACE


// 雷达目标基础数据
struct RadarItemData {
    // 目标编号
    QString id;
    // 目标名称
    QString name;
    // 目标类型（PT 点目标 / AR 区域目标）
    QString type;

    RadarItemData() : id(""), name(""), type("PT") {}
    RadarItemData(const QString &i, const QString &n, const QString &t = "PT")
        : id(i), name(n), type(t) {}
};


// 点目标输入参数：每个目标独立保存
struct PtInputData {
    // 毁伤要求 P̄
    double damageLevel = 0.9;
    // 单弹毁伤概率 Pk
    double singleShotProb = 0.95;
    // 备份架数
    int backupCount = 1;
};

// 区域目标输入参数：每个目标独立保存
struct ArInputData {
    // 区域面积 KM²
    int area = 100;
    // 估计目标数 M
    double estTargetCount = 3.0;
    // 每目标弹数 n
    int shotsPerTarget = 2;
    // 备份架数
    int backupCount = 2;
};

// 兵力需求计算面板类，支持点目标和区域目标两种模型
class ForceRequirementPanel : public QFrame {
    Q_OBJECT

public:
    // 构造函数
    explicit ForceRequirementPanel(QWidget *parent = nullptr);
    // 析构函数
    ~ForceRequirementPanel() override;

    // ═══════════════════════════════════════════
    // 目标选择 API
    // 自由添加/删除计算目标 pill
    // ═══════════════════════════════════════════

    // 添加一个计算目标 pill
    // @param id    目标编号（如 "PT-01"）
    // @param name  目标名称（如 "东郊制导雷达"）
    // @param type  目标类型（"PT" 点目标 / "AR" 区域目标）
    // @return      目标索引
    int addTarget(const QString &id, const QString &name, const QString &type);

    // 移除指定索引的计算目标 pill
    void removeTarget(int index);

    // 清空所有计算目标 pill
    void clearTargets();

    // 获取计算目标总数
    int targetCount() const;

    // 选中指定索引的计算目标
    void selectTarget(int index);

private slots:
    // 计算过程按钮点击响应
    void onCalculateProcessClicked();
    // 兵力汇总按钮点击响应
    void onTotalForceSummaryClicked();

protected:
    // 事件过滤器：捕获 pill 点击事件
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // UI 指针
    Ui::ForceRequirementPanel *ui;

    // 目标 pill 网格布局（在 scrollWidget 内）
    QGridLayout *m_targetGrid;
    // 目标 pill 可滚动区域
    QScrollArea *m_scrollArea;
    // 滚动区域内容容器
    QWidget *m_scrollWidget;
    // 所有目标 pill 容器
    QList<QPushButton*> m_targetPills;
    // 目标 ID 列表
    QList<QString> m_targetIds;
    // 目标名称列表
    QList<QString> m_targetNames;
    // 目标类型列表（PT 点目标 / AR 区域目标）
    QList<QString> m_targetTypes;

    // 当前选中目标 ID
    QString m_currentTargetId;
    // 当前选中目标类型
    QString m_currentTargetType;
    // 当前毁伤要求 P̄ 值
    double m_currentDamageLevel;
    // 点目标计算结果映射表
    QMap<QString, PtCalcData> m_ptResults;
    // 区域目标计算结果映射表
    QMap<QString, ArCalcData> m_arResults;
    // 点目标输入参数映射表
    QMap<QString, PtInputData> m_ptInputs;
    // 区域目标输入参数映射表
    QMap<QString, ArInputData> m_arInputs;

    // 初始化雷达目标选择区
    void initTargetScrollArea();
    // 初始化信号槽连接
    void setupConnections();
    // 应用技术样式
    void applyTechStyle();
    // 根据目标类型切换页面
    void updatePageForTarget(const QString &targetId, const QString &targetType);
    // 填充兵力汇总表格
    void populateSummaryTable();
    // 初始化点目标输入参数信号槽
    void setupPtInputConnections();
    // 初始化区域目标输入参数信号槽
    void setupArInputConnections();
    // 更新点目标计算结果
    void updatePtCalculation();
    // 更新区域目标计算结果
    void updateArCalculation();

    // 保存当前选中目标的输入参数到映射表
    void saveCurrentInput();
    // 从映射表恢复指定目标的输入参数到 UI 控件
    void restoreInput(const QString &id, const QString &type);

    // 创建一个目标 pill 控件
    QPushButton *createTargetPill(const QString &id, const QString &name, const QString &type);

    // 填充默认计算目标
    void setupDefaultTargets();

    // 根据 pill 数量调整目标容器高度
    void updateTargetGroupHeight();
};

#endif //RZSIM_ANTI_RADIATION_UAV_FORCEREQUirementPANEL_H
