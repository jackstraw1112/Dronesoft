// ============================================================================
// 右侧资源态势面板头文件
// ============================================================================
// 功能描述：
//   显示关键指标、可用兵力、威胁清单和气象简报
//   提供实时态势感知和资源管理界面
// ============================================================================

#ifndef RIGHTSIDEPANEL_H
#define RIGHTSIDEPANEL_H

#include <QWidget>
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QList>

// ============================================================================
// RightSidePanel - 右侧资源态势面板类
// ============================================================================
// 主要功能：
//   - 显示关键指标（KPI）：点目标、区域目标、出动架次、预期毁伤率
//   - 显示可用兵力：无人机列表及其状态
//   - 显示威胁清单：敌方防空、干扰源等威胁信息
//   - 显示气象简报：风向、能见度、云底高、温度等气象数据
// ============================================================================
class RightSidePanel : public QWidget
{
    Q_OBJECT

public:
    // =========================================================================
    // 构造函数
    // =========================================================================
    explicit RightSidePanel(QWidget *parent = nullptr);

    // =========================================================================
    // 公共接口方法
    // =========================================================================
    
    // 添加无人机卡片
    // 参数：name - 无人机名称，spec - 规格说明，status - 状态文本，isAvailable - 是否可用
    void addUavCard(QString name, QString spec, QString status, bool isAvailable);
    
    // 添加KPI指标卡片
    // 参数：label - 指标名称，value - 指标数值
    void addKpiCard(QString label, QString value);
    
    // 添加威胁项目
    // 参数：name - 威胁名称，range - 威胁范围
    void addThreatItem(QString name, QString range);
    
    // 添加气象卡片
    // 参数：label - 气象项目名称，value - 气象数值
    void addWeatherCard(QString label, QString value);

private:
    // =========================================================================
    // 私有成员变量
    // =========================================================================
    
    QVBoxLayout *m_mainLayout;          // 主布局
    QLabel *m_badgeLabel;               // 状态徽章标签（LIVE）
    QWidget *m_scrollContent;           // 滚动区域内容容器
    QVBoxLayout *m_contentLayout;       // 内容布局
    QScrollArea *m_scrollArea;          // 滚动区域
    
    QList<QFrame*> m_kpiCards;          // KPI卡片列表
    QList<QFrame*> m_uavCards;          // 无人机卡片列表
    QList<QFrame*> m_threatItems;       // 威胁项目列表
    QList<QFrame*> m_weatherCards;      // 气象卡片列表

    // =========================================================================
    // 私有初始化方法
    // =========================================================================
    
    void setupUi();                     // 初始化UI界面
    void setupHeader();                 // 设置头部区域
    void setupKpiSection();             // 设置KPI指标区域
    void setupAssetsSection();          // 设置可用兵力区域
    void setupThreatsSection();         // 设置威胁清单区域
    void setupWeatherSection();         // 设置气象简报区域
};

#endif
