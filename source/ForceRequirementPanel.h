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
#include "StructData.h"


QT_BEGIN_NAMESPACE
namespace Ui { class ForceRequirementPanel; }
QT_END_NAMESPACE


// 雷达目标基础数据
struct RadarItemData {
    QString id;
    QString name;
    QString type;

    RadarItemData() : id(""), name(""), type("PT") {}
    RadarItemData(const QString &i, const QString &n, const QString &t = "PT")
        : id(i), name(n), type(t) {}
};


// 兵力需求计算面板类，支持点目标和区域目标两种模型
class ForceRequirementPanel : public QFrame {
    Q_OBJECT

public:
    explicit ForceRequirementPanel(QWidget *parent = nullptr);
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
    void onCalculateProcessClicked();
    void onTotalForceSummaryClicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::ForceRequirementPanel *ui;

    QGridLayout *m_targetGrid;          // 目标 pill 网格布局
    QList<QPushButton*> m_targetPills;  // 所有目标 pill 容器
    QList<QString> m_targetIds;         // 目标 ID 列表
    QList<QString> m_targetNames;       // 目标名称列表
    QList<QString> m_targetTypes;       // 目标类型列表 (PT / AR)

    QString m_currentTargetId;          // 当前选中目标ID
    QString m_currentTargetType;        // 当前选中目标类型
    double m_currentDamageLevel;        // 当前毁伤要求P̄值
    QMap<QString, PtCalcData> m_ptResults;
    QMap<QString, ArCalcData> m_arResults;

    void setupConnections();
    void applyTechStyle();
    void updatePageForTarget(const QString &targetId, const QString &targetType);
    void populateSummaryTable();
    void setupPtInputConnections();
    void setupArInputConnections();
    void updatePtCalculation();
    void updateArCalculation();

    QPushButton *createTargetPill(const QString &id, const QString &name, const QString &type);
    void setupDefaultTargets();
};

#endif //RZSIM_ANTI_RADIATION_UAV_FORCEREQUirementPANEL_H
