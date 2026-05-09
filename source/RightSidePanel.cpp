// ============================================================================
// 右侧资源态势面板实现文件
// ============================================================================
// 功能描述：
//   实现关键指标、可用兵力、威胁清单和气象简报的显示
//   提供可视化的态势感知界面组件
// ============================================================================

#include "RightSidePanel.h"
#include <QPushButton>
#include <QVariant>

// ============================================================================
// 构造函数
// ============================================================================
// 功能：初始化右侧面板，调用UI设置方法
// 参数：parent - 父窗口指针
// ============================================================================
RightSidePanel::RightSidePanel(QWidget *parent)
    : QFrame(parent)
{
    setupUi();
    applyTechStyle();
}

void RightSidePanel::applyTechStyle()
{
    const QString baseBg = "#0a0e1a";
    const QString panelBg = "#0d1326";
    const QString borderColor = "#1a3a6a";
    const QString accentBlue = "#00b4ff";
    const QString accentCyan = "#00e5ff";
    const QString textPrimary = "#e0e8f0";
    const QString textSecondary = "#7a8ba8";
    const QString groupBoxBg = "#0b1124";
    const QString inputBg = "#0f1a2e";

    setStyleSheet(QString(R"(
        RightSidePanel {
            background-color: %1;
            border: 2px solid %2;
            border-radius: 6px;
        }
        QWidget {
            color: %3;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
        }
    )").arg(baseBg).arg(borderColor).arg(textPrimary));
}

// ============================================================================
// setupUi - 初始化UI界面
// ============================================================================
// 功能：设置面板的整体布局、样式和滚动区域
// 创建头部、KPI区域、兵力区域、威胁区域和气象区域
// ============================================================================
void RightSidePanel::setupUi()
{
    // 设置面板对象名称和背景样式
    setObjectName("rightSidePanel");

    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 设置头部区域
    setupHeader();

    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("rightScrollArea");
    m_scrollArea->setStyleSheet(
        "QScrollArea#rightScrollArea {"
        "   background-color: #0d1326;"
        "   border: none;"
        "}"
        "QScrollArea#rightScrollArea > QWidget {"
        "   background-color: #0d1326;"
        "}"
        "QScrollArea#rightScrollArea QScrollBar:vertical {"
        "   background-color: #0d1326;"
        "   width: 6px;"
        "   margin: 0px;"
        "}"
        "QScrollArea#rightScrollArea QScrollBar::handle:vertical {"
        "   background-color: #1a3a6a;"
        "   border-radius: 3px;"
        "   min-height: 20px;"
        "}"
        "QScrollArea#rightScrollArea QScrollBar::handle:vertical:hover {"
        "   background-color: #00b4ff;"
        "}"
        "QScrollArea#rightScrollArea QScrollBar::add-line:vertical,"
        "QScrollArea#rightScrollArea QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}"
    );
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 创建滚动内容容器
    m_scrollContent = new QWidget(m_scrollArea);
    m_scrollContent->setObjectName("scrollContent");
    m_scrollContent->setStyleSheet(
        "QWidget#scrollContent {"
        "   background-color: #0d1326;"
        "}"
    );

    // 创建内容布局
    m_contentLayout = new QVBoxLayout(m_scrollContent);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(0);

    // 设置各个功能区域
    setupKpiSection();
    setupAssetsSection();
    //setupThreatsSection();
    //setupWeatherSection();

    // 添加弹性空间
    m_contentLayout->addStretch();

    // 将内容容器设置到滚动区域
    m_scrollArea->setWidget(m_scrollContent);

    // 将滚动区域添加到主布局
    m_mainLayout->addWidget(m_scrollArea);
}

// ============================================================================
// setupHeader - 设置头部区域
// ============================================================================
// 功能：创建面板头部，包含标题和状态徽章
// 显示"资源/态势"标题和"LIVE"状态标识
// ============================================================================
void RightSidePanel::setupHeader()
{
    // 创建头部框架
    QFrame *headerFrame = new QFrame(this);
    headerFrame->setObjectName("headerFrame");
    headerFrame->setMinimumSize(QSize(0, 44));
    headerFrame->setStyleSheet(
        "QFrame#headerFrame {"
        "   background-color: #0b1124;"
        "   border-bottom: 1px solid #1a3a6a;"
        "}"
    );

    // 创建头部布局
    QHBoxLayout *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(16, 10, 16, 10);

    // 创建标题标签
    QLabel *titleLabel = new QLabel(headerFrame);
    titleLabel->setText("资源 / 态势");
    titleLabel->setStyleSheet(
        "QLabel {"
        "   color: #00b4ff;"
        "   font-weight: bold;"
        "   font-size: 18px;"
        "   letter-spacing: 2px;"
        "   border-left: 4px solid #00b4ff;"
        "   padding-left: 10px;"
        "}"
    );

    // 创建状态徽章标签
    m_badgeLabel = new QLabel(headerFrame);
    m_badgeLabel->setText("LIVE");
    m_badgeLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 14px;"
        "   color: #7a8ba8;"
        "   letter-spacing: 1px;"
        "}"
    );

    // 添加控件到布局
    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(m_badgeLabel);

    // 将头部框架添加到主布局
    m_mainLayout->addWidget(headerFrame);
}

// ============================================================================
// setupKpiSection - 设置KPI指标区域
// ============================================================================
// 功能：创建关键指标显示区域，使用网格布局显示4个KPI卡片
// 显示：点目标、区域目标、出动架次、无人机总数
// ============================================================================
void RightSidePanel::setupKpiSection()
{
    // 创建区域框架
    QFrame *sectionFrame = new QFrame(m_scrollContent);
    sectionFrame->setObjectName("sectionFrame");
    sectionFrame->setStyleSheet(
        "QFrame#sectionFrame {"
        "   background-color: #0d1326;"
        "   border-bottom: 1px solid #1a3a6a;"
        "}"
    );

    // 创建区域布局
    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(16, 14, 16, 14);
    sectionLayout->setSpacing(12);

    // 创建区域标题
    QLabel *sectionTitle = new QLabel(sectionFrame);
    sectionTitle->setText("关键指标 / KPI");
    sectionTitle->setStyleSheet(
        "QLabel {"
        "   font-family: 'Microsoft YaHei', sans-serif;"
        "   font-size: 16px;"
        "   color: #00b4ff;"
        "   letter-spacing: 1.5px;"
        "}"
    );
    sectionLayout->addWidget(sectionTitle);

    // 创建网格布局
    QGridLayout *kpiGrid = new QGridLayout();
    kpiGrid->setSpacing(10);

    // 添加KPI卡片到网格
    addKpiCard("点目标", "3");
    kpiGrid->addWidget(m_kpiCards.last(), 0, 0);

    addKpiCard("区域目标", "2");
    kpiGrid->addWidget(m_kpiCards.last(), 0, 1);

    addKpiCard("出动架次", "12");
    kpiGrid->addWidget(m_kpiCards.last(), 1, 0);

    addKpiCard("无人机总数", "240");
    kpiGrid->addWidget(m_kpiCards.last(), 1, 1);

    sectionLayout->addLayout(kpiGrid);

    m_contentLayout->addWidget(sectionFrame);
}

// ============================================================================
// addKpiCard - 添加KPI指标卡片
// ============================================================================
// 功能：创建一个KPI指标卡片，显示指标名称和数值
// 参数：label - 指标名称，value - 指标数值
// ============================================================================
void RightSidePanel::addKpiCard(QString label, QString value)
{
    // 创建KPI卡片框架
    QFrame *kpiCard = new QFrame(m_scrollContent);
    kpiCard->setObjectName("kpiCard");
    kpiCard->setStyleSheet(
        "QFrame#kpiCard {"
        "   background-color: #0b1124;"
        "   border: 1px solid #1a3a6a;"
        "   padding: 12px 14px;"
        "}"
    );

    // 创建卡片布局
    QVBoxLayout *cardLayout = new QVBoxLayout(kpiCard);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(6);
    cardLayout->setAlignment(Qt::AlignCenter);

    // 创建指标名称标签
    QLabel *kpiLabel = new QLabel(kpiCard);
    kpiLabel->setText(label);
    kpiLabel->setAlignment(Qt::AlignCenter);
    kpiLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 10px;"
        "   color: #7a8ba8;"
        "   letter-spacing: 1px;"
        "}"
    );

    // 创建指标数值标签
    QLabel *kpiValue = new QLabel(kpiCard);
    kpiValue->setText(value);
    kpiValue->setAlignment(Qt::AlignCenter);
    kpiValue->setStyleSheet(
        "QLabel {"
        "   font-size: 15px;"
        "   font-weight: bold;"
        "   color: #00e5ff;"
        "   font-family: 'Microsoft YaHei', monospace;"
        "}"
    );

    // 添加控件到布局
    cardLayout->addWidget(kpiLabel);
    cardLayout->addWidget(kpiValue);

    // 将卡片添加到列表
    m_kpiCards.append(kpiCard);
    m_kpiValueLabels.append(kpiValue);
}

// ============================================================================
// setupAssetsSection - 设置可用兵力区域
// ============================================================================
// 功能：创建可用兵力显示区域，显示无人机列表及其状态
// 包含：HF-ARM-200A、HF-ARM-200B、HF-ARM-300、HF-EW-100等无人机信息
// ============================================================================
void RightSidePanel::setupAssetsSection()
{
    // 创建区域框架
    QFrame *sectionFrame = new QFrame(m_scrollContent);
    sectionFrame->setObjectName("sectionFrame");
    sectionFrame->setStyleSheet(
        "QFrame#sectionFrame {"
        "   background-color: #0d1326;"
        "   border-bottom: 1px solid #1a3a6a;"
        "}"
    );

    // 创建区域布局
    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(16, 14, 16, 14);
    sectionLayout->setSpacing(8);

    // 创建区域标题
    QLabel *sectionTitle = new QLabel(sectionFrame);
    sectionTitle->setText("可用兵力 / ASSETS");
    sectionTitle->setStyleSheet(
        "QLabel {"
        "   font-family: 'Microsoft YaHei', sans-serif;"
        "   font-size: 16px;"
        "   color: #00b4ff;"
        "   letter-spacing: 1.5px;"
        "}"
    );
    sectionLayout->addWidget(sectionTitle);

    // 存储布局指针，供 setUavResources 动态更新
    m_assetsSectionLayout = sectionLayout;

    m_contentLayout->addWidget(sectionFrame);
}

// ============================================================================
// addUavCard - 添加无人机卡片
// ============================================================================
// 功能：创建一个无人机信息卡片，显示无人机名称、规格和状态
// 参数：name - 无人机名称，spec - 规格说明，status - 状态文本，isAvailable - 是否可用
// ============================================================================
void RightSidePanel::addUavCard(QString name, QString spec, QString status, bool isAvailable)
{
    // 创建无人机卡片框架
    QFrame *uavCard = new QFrame(m_scrollContent);
    uavCard->setObjectName("uavCard");
    uavCard->setStyleSheet(
        "QFrame#uavCard {"
        "   background-color: #0b1124;"
        "   border: 1px solid #1a3a6a;"
        "   padding: 10px 12px;"
        "   margin-bottom: 8px;"
        "}"
    );

    // 创建卡片布局
    QHBoxLayout *cardLayout = new QHBoxLayout(uavCard);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(10);

    // 创建无人机图标
    QLabel *uavIcon = new QLabel(uavCard);
    uavIcon->setText("✈");
    uavIcon->setStyleSheet(
        "QLabel {"
        "   font-size: 24px;"
        "   color: #00e5ff;"
        "   min-width: 36px;"
        "}"
    );

    // 创建信息布局
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    infoLayout->setAlignment(Qt::AlignLeft);

    // 创建无人机名称标签
    QLabel *uavName = new QLabel(uavCard);
    uavName->setText(name);
    uavName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    uavName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    uavName->setStyleSheet(
        "QLabel {"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   color: #e0e8f0;"
        "}"
    );

    // 创建无人机规格标签
    QLabel *uavSpec = new QLabel(uavCard);
    uavSpec->setText(spec);
    uavSpec->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    uavSpec->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    uavSpec->setStyleSheet(
        "QLabel {"
        "   font-size: 12px;"
        "   color: #7a8ba8;"
        "}"
    );

    infoLayout->addWidget(uavName);
    infoLayout->addWidget(uavSpec);

    // 创建状态标签
    QLabel *statusLabel = new QLabel(uavCard);
    statusLabel->setText(status);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setFixedSize(80, 28);

    // 根据状态设置不同的样式
    if (status.contains("已分配")) {
        // 已分配状态 - 蓝色
        statusLabel->setStyleSheet(
            "QLabel {"
            "   font-size: 13px;"
            "   color: #00b4ff;"
            "   background-color: rgba(0, 180, 255, 0.15);"
            "   border: 1px solid #1a3a6a;"
            "   padding: 3px 8px;"
            "   border-radius: 3px;"
            "}"
        );
    } else if (status == "就绪") {
        // 就绪状态 - 青色
        statusLabel->setStyleSheet(
            "QLabel {"
            "   font-size: 13px;"
            "   color: #00e5ff;"
            "   background-color: rgba(0, 229, 255, 0.15);"
            "   border: 1px solid #1a3a6a;"
            "   padding: 3px 8px;"
            "   border-radius: 3px;"
            "}"
        );
    } else {
        // 其他状态 - 灰色
        statusLabel->setStyleSheet(
            "QLabel {"
            "   font-size: 13px;"
            "   color: #7a8ba8;"
            "   background-color: transparent;"
            "   border: 1px solid #1a3a6a;"
            "   padding: 3px 8px;"
            "   border-radius: 3px;"
            "}"
        );
    }

    // 添加控件到布局
    cardLayout->addWidget(uavIcon);
    cardLayout->addLayout(infoLayout);
    cardLayout->addWidget(statusLabel);

    // 将卡片添加到列表
    m_uavCards.append(uavCard);
}

// ============================================================================
// setupThreatsSection - 设置威胁清单区域
// ============================================================================
// 功能：创建威胁清单显示区域，显示敌方威胁信息
// 包含：SAM-1、SAM-2、AAA高炮阵地、EW干扰源等威胁
// ============================================================================
void RightSidePanel::setupThreatsSection()
{
    // 创建区域框架
    QFrame *sectionFrame = new QFrame(m_scrollContent);
    sectionFrame->setObjectName("sectionFrame");
    sectionFrame->setStyleSheet(
        "QFrame#sectionFrame {"
        "   background-color: #0d1326;"
        "   border-bottom: 1px solid #1a3a6a;"
        "}"
    );

    // 创建区域布局
    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(16, 14, 16, 14);
    sectionLayout->setSpacing(10);

    // 创建区域标题
    QLabel *sectionTitle = new QLabel(sectionFrame);
    sectionTitle->setText("威胁清单 / THREATS");
    sectionTitle->setStyleSheet(
        "QLabel {"
        "   font-family: 'Microsoft YaHei', sans-serif;"
        "   font-size: 16px;"
        "   color: #00b4ff;"
        "   letter-spacing: 1.5px;"
        "}"
    );
    sectionLayout->addWidget(sectionTitle);

    // 添加威胁项目
    addThreatItem("SAM-1 中程地空", "35 KM");
    sectionLayout->addWidget(m_threatItems.last());

    addThreatItem("SAM-2 末端防御", "25 KM");
    sectionLayout->addWidget(m_threatItems.last());

    addThreatItem("AAA 高炮阵地", "8 KM");
    sectionLayout->addWidget(m_threatItems.last());

    addThreatItem("EW 干扰源", "FREQ G/H");
    sectionLayout->addWidget(m_threatItems.last());

    m_contentLayout->addWidget(sectionFrame);
}

// ============================================================================
// addThreatItem - 添加威胁项目
// ============================================================================
// 功能：创建一个威胁信息项，显示威胁名称和范围
// 参数：name - 威胁名称，range - 威胁范围或频率
// ============================================================================
void RightSidePanel::addThreatItem(QString name, QString range)
{
    // 创建威胁项目框架
    QFrame *threatItem = new QFrame(m_scrollContent);
    threatItem->setObjectName("threatItem");
    threatItem->setStyleSheet(
        "QFrame#threatItem {"
        "   background-color: transparent;"
        "   padding: 4px 0;"
        "}"
    );

    // 创建项目布局
    QHBoxLayout *itemLayout = new QHBoxLayout(threatItem);
    itemLayout->setContentsMargins(0, 0, 0, 0);
    itemLayout->setSpacing(10);

    // 创建威胁名称标签
    QLabel *threatName = new QLabel(threatItem);
    threatName->setText(name);
    threatName->setStyleSheet(
        "QLabel {"
        "   font-size: 15px;"
        "   color: #e0e8f0;"
        "}"
    );

    // 创建威胁范围标签
    QLabel *threatRange = new QLabel(threatItem);
    threatRange->setText(range);
    threatRange->setAlignment(Qt::AlignRight);
    threatRange->setStyleSheet(
        "QLabel {"
        "   font-size: 15px;"
        "   font-family: 'Microsoft YaHei', monospace;"
        "   color: #ff4d4d;"
        "}"
    );

    // 添加控件到布局
    itemLayout->addWidget(threatName);
    itemLayout->addWidget(threatRange);

    // 将项目添加到列表
    m_threatItems.append(threatItem);
}

// ============================================================================
// setupWeatherSection - 设置气象简报区域
// ============================================================================
// 功能：创建气象简报显示区域，使用网格布局显示4个气象卡片
// 显示：风向/风速、能见度、云底高、温度
// ============================================================================
void RightSidePanel::setupWeatherSection()
{
    // 创建区域框架
    QFrame *sectionFrame = new QFrame(m_scrollContent);
    sectionFrame->setObjectName("sectionFrame");
    sectionFrame->setStyleSheet(
        "QFrame#sectionFrame {"
        "   background-color: #0d1326;"
        "   border-bottom: 1px solid #1a3a6a;"
        "}"
    );

    // 创建区域布局
    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(16, 14, 16, 14);
    sectionLayout->setSpacing(12);

    // 创建区域标题
    QLabel *sectionTitle = new QLabel(sectionFrame);
    sectionTitle->setText("气象简报 / WX");
    sectionTitle->setStyleSheet(
        "QLabel {"
        "   font-family: 'Microsoft YaHei', sans-serif;"
        "   font-size: 16px;"
        "   color: #00b4ff;"
        "   letter-spacing: 1.5px;"
        "}"
    );
    sectionLayout->addWidget(sectionTitle);

    // 创建网格布局
    QGridLayout *weatherGrid = new QGridLayout();
    weatherGrid->setSpacing(8);

    // 添加气象卡片到网格
    addWeatherCard("风向 / 风速", "SW · 4.2 m/s");
    weatherGrid->addWidget(m_weatherCards.last(), 0, 0);

    addWeatherCard("能见度", "12 KM");
    weatherGrid->addWidget(m_weatherCards.last(), 0, 1);

    addWeatherCard("云底高", "2400 M");
    weatherGrid->addWidget(m_weatherCards.last(), 1, 0);

    addWeatherCard("温度", "18 °C");
    weatherGrid->addWidget(m_weatherCards.last(), 1, 1);

    sectionLayout->addLayout(weatherGrid);

    m_contentLayout->addWidget(sectionFrame);
}

// ============================================================================
// addWeatherCard - 添加气象卡片
// ============================================================================
// 功能：创建一个气象信息卡片，显示气象项目名称和数值
// 参数：label - 气象项目名称，value - 气象数值
// ============================================================================
void RightSidePanel::addWeatherCard(QString label, QString value)
{
    // 创建气象卡片框架
    QFrame *weatherCard = new QFrame(m_scrollContent);
    weatherCard->setObjectName("weatherCard");
    weatherCard->setStyleSheet(
        "QFrame#weatherCard {"
        "   background-color: #0b1124;"
        "   border: 1px solid #1a3a6a;"
        "   padding: 10px 12px;"
        "}"
    );

    // 创建卡片布局
    QVBoxLayout *cardLayout = new QVBoxLayout(weatherCard);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(4);
    cardLayout->setAlignment(Qt::AlignCenter);

    // 创建气象项目名称标签
    QLabel *weatherLabel = new QLabel(weatherCard);
    weatherLabel->setText(label);
    weatherLabel->setAlignment(Qt::AlignCenter);
    weatherLabel->setStyleSheet(
        "QLabel {"
        "   font-size: 13px;"
        "   color: #7a8ba8;"
        "   letter-spacing: 1px;"
        "}"
    );

    // 创建气象数值标签
    QLabel *weatherValue = new QLabel(weatherCard);
    weatherValue->setText(value);
    weatherValue->setAlignment(Qt::AlignCenter);
    weatherValue->setStyleSheet(
        "QLabel {"
        "   font-size: 15px;"
        "   font-family: 'Microsoft YaHei', monospace;"
        "   color: #00e5ff;"
        "   margin-top: 4px;"
        "}"
    );

    // 添加控件到布局
    cardLayout->addWidget(weatherLabel);
    cardLayout->addWidget(weatherValue);

    // 将卡片添加到列表
    m_weatherCards.append(weatherCard);
}

// ============================================================================
// setUavResources - 从资源池更新可用兵力显示
// ============================================================================
void RightSidePanel::setUavResources(const QList<UavResource> &resources)
{
    clearUavCards();
    if (!m_assetsSectionLayout)
        return;

    // 按型号分组统计
    QMap<QString, QPair<int, int>> groupStats;
    for (const UavResource &res : resources)
    {
        auto &stats = groupStats[res.typeName];
        stats.first++;   // totalCount
        if (res.isAvailable)
            stats.second++; // availableCount
    }

    for (auto it = groupStats.cbegin(); it != groupStats.cend(); ++it)
    {
        const QString &typeName = it.key();
        int totalCount = it.value().first;
        int availableCount = it.value().second;

        QString spec = QString("%1 架可用").arg(availableCount);

        QString statusText;
        if (availableCount < totalCount)
            statusText = QStringLiteral("已分配 %1").arg(totalCount - availableCount);
        else
            statusText = QStringLiteral("就绪");

        addUavCard(typeName, spec, statusText, true);
        m_assetsSectionLayout->addWidget(m_uavCards.last());
    }
}

// ============================================================================
// setKpiValue - 更新指定索引的KPI数值
// ============================================================================
void RightSidePanel::setKpiValue(int index, const QString &value)
{
    if (index >= 0 && index < m_kpiValueLabels.size())
        m_kpiValueLabels[index]->setText(value);
}

// ============================================================================
// clearUavCards - 清空所有无人机卡片
// ============================================================================
void RightSidePanel::clearUavCards()
{
    for (QFrame *card : m_uavCards)
    {
        if (m_assetsSectionLayout)
            m_assetsSectionLayout->removeWidget(card);
        delete card;
    }
    m_uavCards.clear();
}
