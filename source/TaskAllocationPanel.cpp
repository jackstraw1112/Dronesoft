//
// Created by Administrator on 2026/5/6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_TaskAllocationPanel.h" resolved

#include "TaskAllocationPanel.h"
#include "ui_TaskAllocationPanel.h"
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QRadioButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QButtonGroup>
#include <QScrollArea>
#include <QFrame>


// ═══════════════════════════════════════════════════════════
// 颜色常量
// 定义深色科技风 UI 的统一调色板，各方法通过引用这些常量
// 保持全局色彩一致性，避免硬编码颜色值散落各处
// ═══════════════════════════════════════════════════════════
static const char *kBaseBg       = "#0a0e1a";   // 最底层背景色（面板外）
static const char *kPanelBg      = "#0d1326";   // 面板背景色
static const char *kBorderColor  = "#1a3a6a";   // 默认边框色
static const char *kAccentBlue   = "#00b4ff";   // 蓝色强调色
static const char *kAccentCyan   = "#00e5ff";   // 青色强调色（高亮）
static const char *kTextPrimary  = "#e0e8f0";   // 主文字色
static const char *kTextSec      = "#7a8ba8";   // 次要文字色
static const char *kInputBg      = "#0f1a2e";   // 输入控件背景
static const char *kInputBorder  = "#1a3a6a";   // 输入控件边框
static const char *kHoverBorder  = "#00b4ff";   // 悬停高亮边框
static const char *kGroupBoxBg   = "#0b1124";   // GroupBox 背景
static const char *kGroupBoxTitle= "#00b4ff";   // GroupBox 标题色


// ═══════════════════════════════════════════════════════════
// 算法权重配置
// 3 种求解算法对应的 4 项目标函数权重
// 排列顺序：W₁ 毁伤效能, W₂ 时效优先, W₃ 兵力成本, W₄ 突防风险
// ═══════════════════════════════════════════════════════════
static const int kWeightProfiles[][4] = {
    {40, 30, 20, 10},   // [0] Hungarian — 毁伤优先，精确指派
    {25, 25, 25, 25},   // [1] GA — 均衡分配，多目标优化
    {15, 25, 40, 20},   // [2] CNP — 成本优先，分布式投标
};
static const int kWeightCount = 4;          // 权重个数
static const int kAnimDuration = 1500;       // 动画持续毫秒数（缓慢过渡）
static const int kAnimInterval = 16;          // 定时器间隔 ms（≈60fps 平滑更新）


// ═══════════════════════════════════════════════════════════
// 构造 / 析构
// ═══════════════════════════════════════════════════════════
TaskAllocationPanel::TaskAllocationPanel(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::TaskAllocationPanel)
    , m_algGroup(new QButtonGroup(this))           // 算法 radio 互斥组
    , m_weightTimer(nullptr)
    , m_weightElapsed(0)
    , m_weightBars{}
    , m_weightLabels{}
    , m_metricGrid(nullptr)
    , m_altPlanLayout(nullptr)
    , m_allocLayout(nullptr)
    , m_allocScrollArea(nullptr)
    , m_allocScrollLayout(nullptr)
{
    // 加载 .ui 文件定义的静态控件
    ui->setupUi(this);

    // 应用全局深色科技风样式
    applyTechStyle();

    // 初始化各动态组合控件组（调用公共 API 填入默认数据）
    setupAlgorithmGroup();
    setupConstraintGroup();
    setupMetricsGroup();
    setupAllocationResult();

    setupWeightControls();

    // 算法切换 → 更新权重动画
    connect(m_algGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this, &TaskAllocationPanel::onAlgChanged);

    // 求解按钮 → 按当前算法生成分配方案
    connect(ui->pushButton, &QPushButton::clicked, this, &TaskAllocationPanel::onSolveClicked);
}

TaskAllocationPanel::~TaskAllocationPanel() {
    delete ui;
}


// ═══════════════════════════════════════════════════════════
// 权重控件初始化
// 修正 .ui 中目标函数权重相关控件的文本和样式
// ═══════════════════════════════════════════════════════════
void TaskAllocationPanel::setupWeightControls()
{
    // 顶栏编辑框设为只读
    ui->lineEdit->setReadOnly(true);

    // 修正权重名称标签：.ui 文件中 label_2 ~ label_5 的 text 均为
    // "毁伤效能W1"，需要替换为正确的带下标的名称
    QStringList weightNames = {"毁伤效能 W₁", "时效优先 W₂", "兵力成本 W₃", "突防风险 W₄"};
    QStringList weightLabels = {"label_2", "label_3", "label_4", "label_5"};
    for (int i = 0; i < weightLabels.size(); ++i) {
        QLabel *lbl = findChild<QLabel*>(weightLabels[i]);
        if (lbl) lbl->setText(weightNames[i]);
    }

    // 为每个进度条设置不同颜色和值，同时缓存指针供动画使用
    // 权重值分别为 0.40, 0.30, 0.20, 0.10
    // 颜色：毁伤→绿, 时效→青, 成本→琥珀, 风险→红
    QStringList progressColors = {"#00e676", "#00e5ff", "#ffb300", "#ff3b3b"};
    QStringList progressNames;
    progressNames << "progressBar_4" << "progressBar_3" << "progressBar_2" << "progressBar";
    QStringList weightValues = {"0.40", "0.30", "0.20", "0.10"};
    QStringList valueLabels = {"label_6", "label_7", "label_8", "label_9"};
    for (int i = 0; i < progressNames.size(); ++i) {
        m_weightBars[i] = findChild<QProgressBar*>(progressNames[i]);
        if (m_weightBars[i]) {
            m_weightBars[i]->setStyleSheet(QString(R"(
                QProgressBar {
                    background-color: %1;
                    border: 1px solid %2;
                    border-radius: 3px;
                    text-align: center;
                    color: %3;
                    font-size: 11px;
                    min-height: 18px;
                }
                QProgressBar::chunk {
                    background-color: %4;
                    border-radius: 2px;
                }
            )").arg(kInputBg).arg(kBorderColor).arg(kTextPrimary).arg(progressColors[i]));
            m_weightBars[i]->setValue(weightValues[i].remove('.').toInt());
        }
        m_weightLabels[i] = findChild<QLabel*>(valueLabels[i]);
        if (m_weightLabels[i]) m_weightLabels[i]->setText(weightValues[i]);
    }
}


// ═══════════════════════════════════════════════════════════
// 算法切换
// 当用户点选不同算法 radio 时触发，启动权重平滑过渡
// ═══════════════════════════════════════════════════════════
void TaskAllocationPanel::onAlgChanged(int id)
{
    if (id < 0 || id >= algorithmCount()) return;

    // 从缓存指针读取当前进度条值作为动画起始点，零 findChild
    for (int i = 0; i < kWeightCount; ++i) {
        m_weightFrom[i] = m_weightBars[i] ? m_weightBars[i]->value() : 0;
        m_weightTo[i] = kWeightProfiles[id][i];
    }

    animateWeights();
}

// ═══════════════════════════════════════════════════════════
// 权重平滑动画
// 使用 QTimer (60fps) 驱动，在 kAnimDuration 毫秒内
// 从 m_weightFrom[] 经 OutQuad 缓出插值到 m_weightTo[]
// ═══════════════════════════════════════════════════════════
void TaskAllocationPanel::animateWeights()
{
    if (m_weightTimer) {
        m_weightTimer->stop();
        m_weightTimer->deleteLater();
        m_weightTimer = nullptr;
    }
    m_weightElapsed = 0;

    m_weightTimer = new QTimer(this);
    m_weightTimer->setInterval(kAnimInterval);
    connect(m_weightTimer, &QTimer::timeout, this, [this]() {
        m_weightElapsed += kAnimInterval;
        double t = qMin(1.0, static_cast<double>(m_weightElapsed) / kAnimDuration);
        double eased = t * (2.0 - t);  // OutQuad 缓出
        for (int i = 0; i < kWeightCount; ++i) {
            int cur = qRound(m_weightFrom[i] + (m_weightTo[i] - m_weightFrom[i]) * eased);
            if (m_weightBars[i]) m_weightBars[i]->setValue(cur);
            if (m_weightLabels[i]) m_weightLabels[i]->setText(QString::number(cur / 100.0, 'f', 2));
        }
        if (t >= 1.0) {
            m_weightTimer->stop();
            m_weightTimer->deleteLater();
            m_weightTimer = nullptr;
        }
    });
    m_weightTimer->start();
}


// ═══════════════════════════════════════════════════════════
// 兵力数据注入
// 接收来自 ForceRequirementPanel（step2）的目标数据和计算结果，
// 存储为内部成员供求解按钮生成方案使用。
// ═══════════════════════════════════════════════════════════
void TaskAllocationPanel::setForceData(const QList<ForceTargetData> &targets,
                                        const QMap<QString, PtCalcData> &ptResults,
                                        const QMap<QString, ArCalcData> &arResults)
{
    m_forceTargets = targets;
    m_forcePtResults = ptResults;
    m_forceArResults = arResults;

    // 更新顶栏提示信息：显示输入规模
    int totalAircraft = 0;
    for (const auto &t : m_forceTargets)
        totalAircraft += t.aircraftCount;
    ui->label->setText(QString("输入兵力:%1UAV ×%2TGT")
                           .arg(totalAircraft)
                           .arg(m_forceTargets.size()));
}


// ═══════════════════════════════════════════════════════════
// 求解按钮
// 当用户点击"求解"按钮时调用，读取当前选中的算法，
// 委托 generateAllocationResult() 重新生成全部分配方案。
// ═══════════════════════════════════════════════════════════
void TaskAllocationPanel::onSolveClicked()
{
    int alg = currentAlgorithm();
    if (alg < 0) {
        // 无选中算法时默认取第一个
        alg = 0;
        if (alg < m_algRadios.size())
            m_algRadios[alg]->setChecked(true);
    }
    generateAllocationResult();
}


// ═══════════════════════════════════════════════════════════
// 生成分配方案（核心）
// 根据当前选中的算法索引，计算不同权重的兵力分配结果。
// 三种算法产生不同的总架数，体现"不同算法计算架数不一致"。
//
// 算法差异说明：
//   [0] Hungarian（FAST） — 毁伤优先，精确指派 → 总架数 = 需求总和（最省）
//   [1] GA（SLOW）        — 均衡分配，多目标优化 → 总架数 = 需求总和 × 1.15（中等）
//   [2] CNP（DIST）       — 成本优先，分布式投标 → 总架数 = 需求总和 × 1.30（最多）
// ═══════════════════════════════════════════════════════════
void TaskAllocationPanel::generateAllocationResult()
{
    int alg = currentAlgorithm();
    if (alg < 0) alg = 0;
    if (alg >= algorithmCount()) alg = 0;

    // ── 1. 确定目标数据 ──
    // 优先使用 ForceRequirementPanel 注入的真实数据；
    // 若无注入则使用算法默认的权重配置生成演示数据。
    QList<ForceTargetData> targets = m_forceTargets;
    bool hasForceData = !targets.isEmpty();

    if (!hasForceData) {
        // 无兵力数据时，按当前算法权重构造演示目标
        const int *w = kWeightProfiles[alg];
        int sumW = w[0] + w[1] + w[2] + w[3];
        int base = (alg == 0) ? 3 : (alg == 1) ? 4 : 5;
        targets.append(ForceTargetData("PT-01", "东郊制导雷达", "PT", base, "P1"));
        targets.append(ForceTargetData("PT-02", "东郊火控雷达", "PT", base, "P1"));
        targets.append(ForceTargetData("AR-01", "南部防空区",   "AR", base + 1, "P2"));
    }

    // ── 2. 按算法计算总架数和各目标分配架数 ──
    //   multiplier: 算法差异系数
    //   Hungarian = 1.00, GA = 1.15, CNP = 1.30
    double multiplier = (alg == 0) ? 1.00 : (alg == 1) ? 1.15 : 1.30;

    // 计算每个目标分配多少架无人机
    struct TargetAlloc {
        QString id;
        QString name;
        QString type;
        int required;          // 原始需求
        int assigned;          // 分配架数（按算法系数调整后）
        int primary;           // 主攻架数
        int backup;            // 备份架数
        QString priority;
    };
    QList<TargetAlloc> allocs;
    int totalAssigned = 0;

    for (const auto &ft : targets) {
        TargetAlloc ta;
        ta.id = ft.id;
        ta.name = ft.name;
        ta.type = ft.type;
        ta.required = ft.aircraftCount;
        ta.priority = ft.priority;

        // 按算法系数计算分配架数，至少与原需求持平或略多
        int assigned = qMax(ft.aircraftCount,
                            static_cast<int>(qRound(ft.aircraftCount * multiplier)));
        // Hungarian 使用精确指派，不多分配
        if (alg == 0) assigned = ft.aircraftCount;

        ta.assigned = assigned;
        // 主攻：前 2/3（至少 1），其余为备份
        ta.primary = qMax(1, assigned * 2 / 3);
        ta.backup = assigned - ta.primary;
        totalAssigned += assigned;
        allocs.append(ta);
    }

    // ── 3. 计算算法指标 ──
    //   目标函数值: Hungarian 最优 ≈0.96, GA 中等 ≈0.92, CNP 略低 ≈0.88
    double fVal[] = {0.96, 0.92, 0.88};
    double fImprove[] = {47.0, 32.0, 18.0};   // 较初始提升百分比
    int coveragePct = 100;                     // 全部目标覆盖
    int metConstraints = 4;
    int totalConstraints = 4;                  // 硬约束满足数
    double cost[] = {2.84, 3.52, 4.18};        // 综合代价
    int solveTimeMs[] = {150, 890, 620};       // 求解耗时
    int iterations[] = {12, 64, 28};           // 迭代次数

    // 指标数值颜色
    const QString colorGood = "#00e676";
    const QString colorWarn = "#ffb300";

    // 更新或添加 6 项指标
    auto setMetric = [&](int idx, const QString &val, const QString &tag, const QString &color) {
        if (idx < metricCount())
            updateMetric(idx, val, tag, color);
    };

    // 按算法覆盖指标卡片（前 6 项预设指标）
    int mCnt = metricCount();

    // ① 目标函数值
    setMetric(0, QString::number(fVal[alg], 'f', 3),
              QString("较初始 +%1%").arg(fImprove[alg], 0, 'f', 0), "#00e5ff");

    // ② 分配覆盖率
    setMetric(1, QString::number(coveragePct),
              QString("%1/%2 目标").arg(targets.size()).arg(targets.size()), colorGood);

    // ③ 硬约束满足
    setMetric(2, QString("%1/%2").arg(metConstraints).arg(totalConstraints),
              "无冲突", colorGood);

    // ④ 使用兵力
    QString sortieColor = (alg == 0) ? colorGood : (alg == 1) ? colorWarn : "#ff3b3b";
    setMetric(3, QString::number(totalAssigned),
              QString("需求 %1").arg(totalAssigned), sortieColor);

    // ⑤ 综合代价
    setMetric(4, QString::number(cost[alg], 'f', 2),
              "兵力 + 风险", (alg == 0) ? colorGood : "#e0e8f0");

    // ⑥ 求解耗时
    setMetric(5, QString::number(solveTimeMs[alg]),
              QString("%1 次迭代").arg(iterations[alg]), "#00e5ff");

    // ── 4. 候选方案 ──
    // 清空旧方案，生成 3 条算法特定的候选方案
    clearAltPlans();

    // 方案 1（当前选中）: 当前算法结果
    addAltPlan(QString("方案 1 · %1").arg(alg == 0 ? "精确指派" : alg == 1 ? "均衡分配" : "分布投标"),
               QString::number(fVal[alg], 'f', 3),
               QString::number(totalAssigned),
               (alg == 0) ? "低" : (alg == 1) ? "中" : "较低", true);

    // 方案 2（备选）: 假设减 1 架
    int alt2Sorties = qMax(totalAssigned - 1, 1);
    double alt2F = fVal[alg] - 0.03;
    addAltPlan("方案 2 · 节省兵力",
               QString::number(alt2F, 'f', 3),
               QString::number(alt2Sorties),
               (alg == 0) ? "中" : (alg == 1) ? "高" : "中", false);

    // 方案 3（备选）: 假设加 2 架
    int alt3Sorties = totalAssigned + 2;
    double alt3F = fVal[alg] + 0.02;
    addAltPlan("方案 3 · 高冗余",
               QString::number(alt3F, 'f', 3),
               QString::number(alt3Sorties),
               (alg == 0) ? "极低" : (alg == 1) ? "低" : "极低", false);

    // ── 5. 编队分配方案 ──
    // 清空旧分配组，为每个目标生成对应的 UAV 编队
    clearAllocGroups();

    int uavCounter = 1;

    for (int ti = 0; ti < allocs.size(); ++ti) {
        const auto &ta = allocs[ti];
        int uavCount = ta.assigned;

        // 根据目标类型生成波段信息
        QStringList bands;
        if (ta.type == "PT") {
            bands << "I-band · 62km" << "I-band · 63km" << "H-band · 71km"
                  << "H-band · 73km" << "X-band · 55km" << "X-band · 58km";
        } else {
            bands << "S-band · 45km" << "S-band · 48km" << "C-band · 52km"
                  << "C-band · 55km" << "L-band · 60km" << "L-band · 62km";
        }

        // 构造 UAV 编队列表
        QList<UavSpec> uavs;
        for (int ui = 0; ui < uavCount; ++ui) {
            UavSpec spec;
            spec.id = QString("UAV-%1").arg(uavCounter++, 3, 10, QChar('0'));
            // 主攻、协同、备份按顺序循环
            if (ui < ta.primary)
                spec.role = (ui == 0) ? "长机" : QString("僚机 %1").arg(ui);
            else
                spec.role = "备份";
            spec.meta = bands[ui % bands.size()];
            uavs.append(spec);
        }

        // 协同方式描述（固定不变）
        QString coordDesc = QString("%1 机同时到达 · 时间协同 · 误差 ±2s").arg(uavCount);

        // 生成 TOT 时刻（递增）
        QString tot = QString("TOT 14:%1:%2")
                          .arg(42 + ti, 2, 10, QChar('0'))
                          .arg(18 + ti * 5, 2, 10, QChar('0'));

        addAllocGroup(ta.id, ta.name, ta.priority, tot, uavs, coordDesc);
    }

    // 统一所有编队分配组卡片高度（取最高卡片的最小高度为统一值）
    int uniformH = 0;
    for (QFrame *f : m_allocFrames)
        uniformH = qMax(uniformH, f->minimumSizeHint().height());
    for (QFrame *f : m_allocFrames)
        f->setMinimumHeight(uniformH);

    // ── 6. 权重动画同步到选中算法的目标值 ──
    if (alg < algorithmCount()) {
        for (int i = 0; i < kWeightCount; ++i) {
            m_weightFrom[i] = m_weightBars[i] ? m_weightBars[i]->value() : 0;
            m_weightTo[i] = kWeightProfiles[alg][i];
        }
        animateWeights();
    }
}


// ═══════════════════════════════════════════════════════════
// 样式
// 对整个面板及其子控件应用深色科技风 QSS
// ═══════════════════════════════════════════════════════════
void TaskAllocationPanel::applyTechStyle()
{
    // 面板自身：深色背景 + 蓝色边框
    setStyleSheet(QString(R"(
        TaskAllocationPanel {
            background-color: %1;
            border: 2px solid %2;
            border-radius: 6px;
        }
        QWidget {
            color: %3;
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
        }
    )").arg(kBaseBg).arg(kBorderColor).arg(kTextPrimary));

    // 顶部工具栏
    ui->widget->setStyleSheet(QString(R"(
        QWidget#widget {
            background-color: %1;
            border-bottom: 1px solid %2;
        }
    )").arg(kPanelBg).arg(kBorderColor));

    // 滚动区域容器
    ui->widget_3->setStyleSheet(QString(R"(
        QWidget#widget_3 {
            background-color: transparent;
        }
    )"));

    // 分配结果容器
    ui->widget_2->setStyleSheet(QString(R"(
        QWidget#widget_2 {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
        }
    )").arg(kGroupBoxBg).arg(kBorderColor));

    // QGroupBox 通用样式
    // 注：margin-top: 20px 为标题留出空间；
    // ::title 子控件使标题呈现按钮式标签效果
    QString groupBoxTechStyle = QString(R"(
        QGroupBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            margin-top: 20px;
            padding: 14px 8px 8px 8px;
            font-weight: bold;
            color: %3;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 2px 10px;
            color: %3;
            font-size: 12px;
            background-color: %4;
            border: 1px solid %2;
            border-radius: 3px;
            margin-left: 0px;
            margin-top: 6px;
        }
    )").arg(kGroupBoxBg).arg(kBorderColor).arg(kGroupBoxTitle).arg(kInputBg);

    for (QGroupBox* gb : findChildren<QGroupBox*>()) {
        gb->setStyleSheet(groupBoxTechStyle);
    }

    // QPushButton 通用样式
    QString buttonTechStyle = QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 4px 12px;
            font-size: 12px;
            font-weight: bold;
            min-height: 28px;
        }
        QPushButton:hover {
            background-color: %4;
            border: 1px solid %5;
            color: %6;
        }
        QPushButton:pressed {
            background-color: %7;
            border: 1px solid %5;
        }
    )").arg(kInputBg).arg(kBorderColor).arg(kTextPrimary)
       .arg("#0f1a3e").arg(kHoverBorder).arg(kAccentCyan)
       .arg("#081020");

    for (QPushButton* btn : findChildren<QPushButton*>()) {
        btn->setStyleSheet(buttonTechStyle);
    }

    // 求解按钮（主按钮）：更大字号 + 更醒目的边框
    ui->pushButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 6px 16px;
            font-size: 14px;
            font-weight: bold;
            min-height: 36px;
        }
        QPushButton:hover {
            background-color: %4;
            border: 1px solid %5;
            color: %6;
        }
        QPushButton:pressed {
            background-color: %7;
        }
    )").arg("#0a1628").arg(kAccentBlue).arg(kAccentCyan)
       .arg("#0f1a3e").arg(kAccentCyan).arg("#ffffff")
       .arg("#060e1a"));

    // QLineEdit 输入框样式
    QString lineEditStyle = QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            color: %3;
            padding: 2px 8px;
            font-size: 12px;
            min-height: 26px;
        }
        QLineEdit:focus {
            border: 1px solid %4;
            background-color: %5;
        }
        QLineEdit:disabled {
            background-color: %6;
            color: %7;
        }
    )").arg(kInputBg).arg(kInputBorder).arg(kTextPrimary)
       .arg(kHoverBorder).arg("#0a1428")
       .arg("#060a14").arg(kTextSec);

    for (QLineEdit* le : findChildren<QLineEdit*>()) {
        le->setStyleSheet(lineEditStyle);
    }

    // 静态标签（label ~ label_9）分角色着色
    // label    → 顶栏标题，青色加粗 13px
    // label_2~5 → 权重名称，主色加粗 12px
    // label_6~9 → 权重数值，青色加粗 12px
    QStringList labelNames = {
        "label", "label_2", "label_3", "label_4", "label_5",
        "label_6", "label_7", "label_8", "label_9"
    };

    for (const QString& name : labelNames) {
        QLabel* label = findChild<QLabel*>(name);
        if (!label) continue;

        QString color = kTextSec;
        int fontSize = 12;
        bool isBold = false;

        if (name == "label") {
            color = kAccentCyan;
            fontSize = 13;
            isBold = true;
        } else if (name == "label_2" || name == "label_3" ||
                   name == "label_4" || name == "label_5") {
            color = kTextPrimary;
            fontSize = 12;
            isBold = true;
        } else if (name == "label_6" || name == "label_7" ||
                   name == "label_8" || name == "label_9") {
            color = kAccentCyan;
            fontSize = 12;
            isBold = true;
        }

        label->setStyleSheet(QString(R"(
            QLabel {
                color: %1;
                font-size: %2px;
                font-weight: %3;
                background: transparent;
                border: none;
                padding: 1px 2px;
            }
        )").arg(color).arg(fontSize).arg(isBold ? "bold" : "normal"));
    }

}



// ═══════════════════════════════════════════════════════════
// 初始设置
// 各 setup 函数通过调用公共 API 方法填入默认数据，
// 外部使用者也可以直接调用 API 进行动态增删改
// ═══════════════════════════════════════════════════════════
void TaskAllocationPanel::setupAlgorithmGroup()
{
    QGroupBox *gb = ui->groupBox;
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(gb->layout());
    if (!layout) {
        layout = new QVBoxLayout(gb);
        gb->setLayout(layout);
    }
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(6);

    // 添加 3 种默认求解算法
    addAlgorithm("匈牙利算法 · Hungarian", "指派问题最优解 · 适用于无人机数 ≈ 目标数", "FAST", "#00e676");
    addAlgorithm("遗传算法 · GA",          "多约束启发式 · 适合复杂场景与多波次",    "SLOW", "#ffb300");
    addAlgorithm("合同网协议 · CNP",        "分布式投标 · 适合动态重分配场景",       "DIST", "#00b4ff");

    layout->addStretch();

    // 默认选中第一个算法
    if (!m_algRadios.isEmpty())
        m_algRadios.first()->setChecked(true);
}

void TaskAllocationPanel::setupConstraintGroup()
{
    QGroupBox *gb = ui->groupBox_3;
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(gb->layout());
    if (!layout) {
        layout = new QVBoxLayout(gb);
        gb->setLayout(layout);
    }
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(5);

    // 添加 5 条默认协同硬约束
    addConstraint("导引头频段匹配", "UAV 导引头频段必须覆盖目标雷达频段", true);
    addConstraint("航程可达",       "UAV 剩余航程 ≥ 出发→目标航路距离",  true);
    addConstraint("同时到达约束",   "同一目标的多机攻击时刻误差 ≤ 3s",    true);
    addConstraint("P1 目标优先满足", "高优先级目标先分配，剩余兵力给 P2", true);
    addConstraint("不分配同型机扎堆","同一目标避免全是同批次 UAV",       false);

    layout->addStretch();
}

void TaskAllocationPanel::setupMetricsGroup()
{
    QGroupBox *gb = ui->groupBox_4;
    QVBoxLayout *outer = qobject_cast<QVBoxLayout*>(gb->layout());
    if (!outer) {
        outer = new QVBoxLayout(gb);
        gb->setLayout(outer);
    }
    outer->setContentsMargins(8, 6, 8, 6);
    outer->setSpacing(6);

    // 指标卡片网格（3 列布局）
    m_metricGrid = new QGridLayout();
    m_metricGrid->setContentsMargins(0, 0, 0, 0);
    m_metricGrid->setSpacing(6);
    outer->addLayout(m_metricGrid);

    // 分隔线
    QLabel *sep = new QLabel(gb);
    sep->setStyleSheet(QString("background-color: %1; max-height: 1px; min-height: 1px; border: none;").arg(kBorderColor));
    outer->addWidget(sep);

    // 候选方案标题
    QLabel *altTitle = new QLabel("▌ 候选方案对比", gb);
    altTitle->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; background: transparent; border: none;").arg(kAccentCyan));
    outer->addWidget(altTitle);

    // 候选方案容器布局（动态行列表）
    m_altPlanLayout = new QVBoxLayout();
    m_altPlanLayout->setContentsMargins(0, 0, 0, 0);
    m_altPlanLayout->setSpacing(4);
    outer->addLayout(m_altPlanLayout);
    outer->addStretch();

    // 添加 6 项默认求解指标
    addMetric("目标函数值", "0.943", "",  "较初始 +47%", kAccentCyan);
    addMetric("分配覆盖率", "100",   "%", "5/5 目标",   "#00e676");
    addMetric("硬约束满足", "4/4",   "",  "无冲突",      "#00e676");
    addMetric("使用兵力",   "11",    "架","需求 22",     "#ffb300");
    addMetric("综合代价",   "2.84",  "",  "兵力 + 风险", kTextPrimary);
    addMetric("求解耗时",   "150",   "ms","12 次迭代",   kAccentCyan);

    // 添加 3 条默认候选方案
    addAltPlan("方案 1 · 当前",    "0.943", "11", "低",   true);
    addAltPlan("方案 2 · 节省兵力","0.891", "9",  "中",   false);
    addAltPlan("方案 3 · 高冗余",  "0.967", "14", "极低", false);
}

void TaskAllocationPanel::setupAllocationResult()
{
    QWidget *container = ui->widget_2;
    m_allocLayout = qobject_cast<QVBoxLayout*>(container->layout());
    if (!m_allocLayout) {
        m_allocLayout = new QVBoxLayout(container);
        container->setLayout(m_allocLayout);
    }
    m_allocLayout->setContentsMargins(0, 0, 0, 0);
    m_allocLayout->setSpacing(6);

    // 分区标题
    QLabel *title = new QLabel("▌ 分配方案 · 编队组织", container);
    title->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; background: transparent; border: none; padding: 2px 0;").arg(kAccentCyan));
    m_allocLayout->addWidget(title);

    // 滚动区：包裹编队分配组卡片
    m_allocScrollArea = new QScrollArea(container);
    m_allocScrollArea->setWidgetResizable(true);
    m_allocScrollArea->setFrameShape(QFrame::NoFrame);
    m_allocScrollArea->setStyleSheet(QString(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical {"
        "  background: %1; width: 6px; margin: 0;"
        "  border-radius: 3px; }"
        "QScrollBar::handle:vertical {"
        "  background: %2; min-height: 30px; border-radius: 3px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    ).arg(kInputBg).arg(kAccentBlue));

    QWidget *scrollContent = new QWidget(m_allocScrollArea);
    scrollContent->setStyleSheet(QString("background-color: %1; border: none;").arg(kPanelBg));
    m_allocScrollArea->viewport()->setStyleSheet(QString("background-color: %1; border: none;").arg(kPanelBg));
    m_allocScrollLayout = new QVBoxLayout(scrollContent);
    m_allocScrollLayout->setContentsMargins(0, 0, 0, 0);
    m_allocScrollLayout->setSpacing(6);
    scrollContent->setLayout(m_allocScrollLayout);
    m_allocScrollArea->setWidget(scrollContent);

    m_allocLayout->addWidget(m_allocScrollArea, 1);   // stretch=1 让滚动区占据剩余空间

    // 底部弹簧：将滚动区向上推，避免卡片沉底
    m_allocLayout->addStretch(0);

    // 添加 2 个默认编队分配组（PT-01 制导雷达、PT-02 火控雷达）
    addAllocGroup("PT-01", "东郊制导雷达", "P1", "TOT 14:42:18",
                  {{"UAV-A01","主攻 1","I-band · 62km"}, {"UAV-A02","主攻 2","I-band · 62km"}, {"UAV-A03","备份","I-band · 64km"}},
                  "3 机同时到达 · 误差 ±2s · 三向夹角 60°");

    addAllocGroup("PT-02", "东郊火控雷达", "P1", "TOT 14:42:18",
                  {{"UAV-A04","主攻 1","H-band · 71km"}, {"UAV-A05","主攻 2","H-band · 71km"}, {"UAV-A06","备份","H-band · 73km"}},
                  "3 机同时到达 · 与 PT-01 同步压制");
}


// ═══════════════════════════════════════════════════════════
// 控件工厂
// 负责创建单个可视化控件并返回指针，但不负责生命周期管理
// 和列表维护，由公共 API 方法调用并管理
// ═══════════════════════════════════════════════════════════
QFrame *TaskAllocationPanel::createAlgCard(const QString &name, const QString &desc,
                                            const QString &badge, const QString &badgeColor, int id)
{
    QGroupBox *gb = ui->groupBox;
    QFrame *card = new QFrame(gb);
    card->setCursor(Qt::PointingHandCursor);
    card->setObjectName("algCard");
    card->setStyleSheet(QString(R"(
        QFrame#algCard {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 0px;
        }
        QFrame#algCard:hover {
            border: 1px solid %3;
            background-color: #0a1428;
        }
    )").arg(kInputBg).arg(kBorderColor).arg(kHoverBorder));

    QHBoxLayout *row = new QHBoxLayout(card);
    row->setContentsMargins(8, 6, 8, 6);
    row->setSpacing(8);

    QRadioButton *radio = new QRadioButton(card);
    radio->setStyleSheet(QString(R"(
        QRadioButton {
            color: %1;
            font-size: 11px;
            font-weight: bold;
            spacing: 6px;
        }
        QRadioButton::indicator {
            width: 14px;
            height: 14px;
            border-radius: 7px;
            border: 2px solid %2;
            background-color: %3;
        }
        QRadioButton::indicator:checked {
            background-color: %4;
            border: 2px solid %4;
        }
    )").arg(kTextPrimary).arg(kBorderColor).arg(kBaseBg).arg(kAccentCyan));

    m_algGroup->addButton(radio, id);
    m_algRadios.append(radio);
    row->addWidget(radio);

    QVBoxLayout *textCol = new QVBoxLayout();
    textCol->setSpacing(1);
    QLabel *nameLabel = new QLabel(name, card);
    nameLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; background: transparent; border: none;").arg(kTextPrimary));
    textCol->addWidget(nameLabel);
    QLabel *descLabel = new QLabel(desc, card);
    descLabel->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent; border: none;").arg(kTextSec));
    textCol->addWidget(descLabel);
    row->addLayout(textCol, 1);

    QLabel *badgeLbl = new QLabel(badge, card);
    badgeLbl->setAlignment(Qt::AlignCenter);
    badgeLbl->setStyleSheet(QString(R"(
        QLabel {
            color: #000000;
            background-color: %1;
            border-radius: 3px;
            padding: 1px 6px;
            font-size: 9px;
            font-weight: bold;
            font-family: 'Consolas', monospace;
            min-width: 32px;
        }
    )").arg(badgeColor));
    row->addWidget(badgeLbl);

    return card;
}

QFrame *TaskAllocationPanel::createConstraintRow(const QString &name, const QString &desc,
                                                  bool checked, int /*id*/)
{
    QGroupBox *gb = ui->groupBox_3;
    QFrame *rowFrame = new QFrame(gb);
    rowFrame->setStyleSheet(QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 4px; }")
                                .arg(kInputBg).arg(kBorderColor));

    QHBoxLayout *row = new QHBoxLayout(rowFrame);
    row->setContentsMargins(8, 6, 8, 6);
    row->setSpacing(8);

    QCheckBox *check = new QCheckBox(rowFrame);
    check->setStyleSheet(QString(R"(
        QCheckBox {
            color: %1;
            font-size: 11px;
            font-weight: bold;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 3px;
            border: 2px solid %2;
            background-color: %3;
        }
        QCheckBox::indicator:checked {
            background-color: %4;
            border: 2px solid %4;
        }
    )").arg(kTextPrimary).arg(kBorderColor).arg(kBaseBg).arg(kAccentCyan));
    check->setChecked(checked);
    m_constraintChecks.append(check);
    row->addWidget(check);

    QVBoxLayout *textCol = new QVBoxLayout();
    textCol->setSpacing(1);
    QLabel *nameLabel = new QLabel(name, rowFrame);
    nameLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; background: transparent; border: none;").arg(kTextPrimary));
    textCol->addWidget(nameLabel);
    QLabel *descLabel = new QLabel(desc, rowFrame);
    descLabel->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent; border: none;").arg(kTextSec));
    textCol->addWidget(descLabel);
    row->addLayout(textCol, 1);

    return rowFrame;
}

QFrame *TaskAllocationPanel::createMetricCard(const QString &label, const QString &value,
                                               const QString &unit, const QString &tag,
                                               const QString &color)
{
    QGroupBox *gb = ui->groupBox_4;
    QFrame *card = new QFrame(gb);
    card->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 0px;
        }
    )").arg(kInputBg).arg(kBorderColor));

    QVBoxLayout *col = new QVBoxLayout(card);
    col->setContentsMargins(8, 6, 8, 6);
    col->setSpacing(2);

    QLabel *lbl = new QLabel(label, card);
    lbl->setStyleSheet(QString("color: %1; font-size: 9px; background: transparent; border: none; letter-spacing: 1px;").arg(kTextSec));
    col->addWidget(lbl);

    QHBoxLayout *valRow = new QHBoxLayout();
    valRow->setSpacing(2);
    QLabel *valLabel = new QLabel(value, card);
    valLabel->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: bold; background: transparent; border: none; font-family: 'Consolas', monospace;").arg(color));
    valRow->addWidget(valLabel);

    QLabel *unitLabel = nullptr;
    if (!unit.isEmpty()) {
        unitLabel = new QLabel(unit, card);
        unitLabel->setStyleSheet(QString("color: %1; font-size: 11px; background: transparent; border: none; font-family: 'Consolas', monospace;").arg(color));
        valRow->addWidget(unitLabel);
    }
    valRow->addStretch();
    col->addLayout(valRow);

    QLabel *tagLabel = new QLabel(tag, card);
    tagLabel->setStyleSheet(QString("color: %1; font-size: 9px; background: transparent; border: none;").arg(kTextSec));
    col->addWidget(tagLabel);

    // 保存子控件指针，供 updateMetric 后续更新数值/颜色使用
    m_metricValues.append(valLabel);
    m_metricUnits.append(unitLabel);
    m_metricTags.append(tagLabel);

    return card;
}

QFrame *TaskAllocationPanel::createAltPlanRow(const QString &name, const QString &fval,
                                               const QString &sorties, const QString &risk,
                                               bool isCurrent)
{
    QGroupBox *gb = ui->groupBox_4;
    QFrame *planRow = new QFrame(gb);
    planRow->setStyleSheet(QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 3px; }").arg(kInputBg).arg(kBorderColor));

    QHBoxLayout *planLayout = new QHBoxLayout(planRow);
    planLayout->setContentsMargins(8, 4, 8, 4);
    planLayout->setSpacing(10);

    QLabel *nameP = new QLabel(name, planRow);
    nameP->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent; border: none; font-weight: bold;").arg(kTextPrimary));
    planLayout->addWidget(nameP);

    QString fcolor = isCurrent ? "#ffb300" : kTextPrimary;
    QLabel *fvalP = new QLabel(QString("F = %1").arg(fval), planRow);
    fvalP->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent; border: none; font-family: 'Consolas', monospace;").arg(fcolor));
    planLayout->addWidget(fvalP);

    QLabel *sortieP = new QLabel(QString("%1 架次").arg(sorties), planRow);
    sortieP->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent; border: none;").arg(kTextSec));
    planLayout->addWidget(sortieP);

    QLabel *riskP = new QLabel(risk, planRow);
    riskP->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent; border: none;").arg(kTextSec));
    planLayout->addWidget(riskP);
    planLayout->addStretch();

    QLabel *statusP = new QLabel(isCurrent ? "已选" : "选用", planRow);
    QString statusColor = isCurrent ? "#ffb300" : kAccentCyan;
    statusP->setStyleSheet(QString("color: %1; font-size: 9px; background: transparent; border: 1px solid %1; border-radius: 3px; padding: 1px 6px;").arg(statusColor));
    planLayout->addWidget(statusP);

    return planRow;
}

QFrame *TaskAllocationPanel::createAllocGroupFrame(const QString &target, const QString &name,
                                                    const QString &priority, const QString &tot,
                                                    const QList<UavSpec> &uavs,
                                                    const QString &coordDesc)
{
    QWidget *container = ui->widget_2;
    QFrame *groupFrame = new QFrame(container);
    groupFrame->setStyleSheet(QString("QFrame#allocGroupFrame {"
        "background-color: %1;"
        "border: 1px solid %2;"
        "border-radius: 4px; }"
    ).arg(kInputBg).arg(kBorderColor));
    groupFrame->setObjectName("allocGroupFrame");

    QVBoxLayout *groupCol = new QVBoxLayout(groupFrame);
    groupCol->setContentsMargins(8, 6, 8, 6);
    groupCol->setSpacing(4);

    // 表头行：目标编号 + 名称 + 优先级标签 + TOT 时刻
    QHBoxLayout *headerRow = new QHBoxLayout();
    headerRow->setSpacing(8);

    QLabel *tgtLabel = new QLabel(target, groupFrame);
    tgtLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; font-family: 'Consolas', monospace; background: transparent; border: none;").arg(kAccentCyan));
    headerRow->addWidget(tgtLabel);

    QLabel *nameLabel = new QLabel(name, groupFrame);
    nameLabel->setStyleSheet(QString("color: %1; font-size: 11px; background: transparent; border: none;").arg(kTextPrimary));
    headerRow->addWidget(nameLabel);

    // 优先级：P1 红色、P2 琥珀色
    QString priColor = (priority == "P1") ? "#ff3b3b" : "#ffb300";
    QLabel *priLabel = new QLabel(priority, groupFrame);
    priLabel->setAlignment(Qt::AlignCenter);
    priLabel->setStyleSheet(QString("color: #ffffff; background-color: %1; border-radius: 3px; padding: 0 6px; font-size: 9px; font-weight: bold;").arg(priColor));
    headerRow->addWidget(priLabel);
    headerRow->addStretch();

    QLabel *totLabel = new QLabel(tot, groupFrame);
    totLabel->setStyleSheet(QString("color: %1; font-size: 9px; font-family: 'Consolas', monospace; background: transparent; border: none;").arg(kTextSec));
    headerRow->addWidget(totLabel);
    groupCol->addLayout(headerRow);

    // 无人机芯片行：每架 UAV 用一个小芯片展示角色 + 编号 + 元信息
    QHBoxLayout *uavRow = new QHBoxLayout();
    uavRow->setSpacing(6);
    for (int u = 0; u < uavs.size(); ++u) {
        const UavSpec &spec = uavs[u];
        QFrame *chip = new QFrame(groupFrame);
        // 前两架为主攻，其余为备份（深色背景 + 红色边框）
        bool isPrimary = (u < 2);
        QString chipBg = isPrimary ? kInputBg : "#1a0a0a";
        QString chipBorder = isPrimary ? kBorderColor : "#5a1a1a";
        chip->setFixedSize(78, 58);
        chip->setStyleSheet(QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 3px; }").arg(chipBg).arg(chipBorder));

        QVBoxLayout *chipCol = new QVBoxLayout(chip);
        chipCol->setContentsMargins(6, 4, 6, 4);
        chipCol->setSpacing(1);

        QString roleColor = isPrimary ? kAccentCyan : "#ff6b35";
        QLabel *roleLbl = new QLabel(spec.role, chip);
        roleLbl->setAlignment(Qt::AlignCenter);
        roleLbl->setStyleSheet(QString("color: %1; font-size: 9px; font-weight: bold; background: transparent; border: none;").arg(roleColor));
        chipCol->addWidget(roleLbl);

        QLabel *idLbl = new QLabel(spec.id, chip);
        idLbl->setAlignment(Qt::AlignCenter);
        idLbl->setStyleSheet(QString("color: %1; font-size: 10px; font-family: 'Consolas', monospace; background: transparent; border: none;").arg(kTextPrimary));
        chipCol->addWidget(idLbl);

        QLabel *metaLbl = new QLabel(spec.meta, chip);
        metaLbl->setAlignment(Qt::AlignCenter);
        metaLbl->setStyleSheet(QString("color: %1; font-size: 8px; background: transparent; border: none;").arg(kTextSec));
        chipCol->addWidget(metaLbl);

        uavRow->addWidget(chip);
    }
    uavRow->addStretch();
    groupCol->addLayout(uavRow);

    // 协同方式条：描述编队中无人机之间的协同关系
    QFrame *coordFrame = new QFrame(groupFrame);
    coordFrame->setStyleSheet(QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 3px; }").arg(kBaseBg).arg(kBorderColor));
    QHBoxLayout *coordRow = new QHBoxLayout(coordFrame);
    coordRow->setContentsMargins(6, 3, 6, 3);
    coordRow->setSpacing(6);

    QLabel *coordTag = new QLabel("协同方式", coordFrame);
    coordTag->setStyleSheet(QString("color: %1; font-size: 9px; background: transparent; border: none; padding: 0 4px;").arg(kAccentCyan));
    coordRow->addWidget(coordTag);

    QLabel *coordVal = new QLabel(coordDesc, coordFrame);
    coordVal->setStyleSheet(QString("color: %1; font-size: 9px; background: transparent; border: none;").arg(kTextSec));
    coordRow->addWidget(coordVal, 1);
    coordRow->addStretch();

    groupCol->addWidget(coordFrame);
    return groupFrame;
}


// ═══════════════════════════════════════════════════════════
// 求解算法 API 实现
// ═══════════════════════════════════════════════════════════
int TaskAllocationPanel::addAlgorithm(const QString &name, const QString &desc,
                                       const QString &badge, const QString &badgeColor)
{
    int id = m_algCards.size();
    QFrame *card = createAlgCard(name, desc, badge, badgeColor, id);
    m_algCards.append(card);

    QGroupBox *gb = ui->groupBox;
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(gb->layout());
    if (layout) {
        // 插入到 stretch 之前（layout->count() - 1 为 stretch 的索引）
        int insertPos = layout->count() - 1;
        if (insertPos < 0) insertPos = 0;
        layout->insertWidget(insertPos, card);
    }
    return id;
}

void TaskAllocationPanel::removeAlgorithm(int index)
{
    if (index < 0 || index >= m_algCards.size()) return;
    QFrame *card = m_algCards.takeAt(index);
    QRadioButton *radio = m_algRadios.takeAt(index);
    m_algGroup->removeButton(radio);
    card->deleteLater();

    // 删除中间项后，后续 radio 的 id 需要重新对齐
    for (int i = index; i < m_algRadios.size(); ++i)
        m_algGroup->setId(m_algRadios[i], i);
}

void TaskAllocationPanel::clearAlgorithms()
{
    for (QFrame *card : m_algCards) card->deleteLater();
    m_algCards.clear();
    m_algRadios.clear();
    for (QAbstractButton *btn : m_algGroup->buttons())
        m_algGroup->removeButton(btn);
}

void TaskAllocationPanel::setCurrentAlgorithm(int index)
{
    if (index >= 0 && index < m_algRadios.size())
        m_algRadios[index]->setChecked(true);
}

int TaskAllocationPanel::currentAlgorithm() const
{
    return m_algGroup->checkedId();
}

int TaskAllocationPanel::algorithmCount() const
{
    return m_algCards.size();
}


// ═══════════════════════════════════════════════════════════
// 协同硬约束 API 实现
// ═══════════════════════════════════════════════════════════
int TaskAllocationPanel::addConstraint(const QString &name, const QString &desc, bool checked)
{
    int id = m_constraintRows.size();
    QFrame *row = createConstraintRow(name, desc, checked, id);
    m_constraintRows.append(row);

    QGroupBox *gb = ui->groupBox_3;
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(gb->layout());
    if (layout) {
        int insertPos = layout->count() - 1; // stretch 前
        if (insertPos < 0) insertPos = 0;
        layout->insertWidget(insertPos, row);
    }
    return id;
}

void TaskAllocationPanel::removeConstraint(int index)
{
    if (index < 0 || index >= m_constraintRows.size()) return;
    QFrame *row = m_constraintRows.takeAt(index);
    QCheckBox *check = m_constraintChecks.takeAt(index);
    delete check;
    row->deleteLater();
}

void TaskAllocationPanel::clearConstraints()
{
    for (QFrame *row : m_constraintRows) row->deleteLater();
    m_constraintRows.clear();
    m_constraintChecks.clear();
}

void TaskAllocationPanel::setConstraintChecked(int index, bool checked)
{
    if (index >= 0 && index < m_constraintChecks.size())
        m_constraintChecks[index]->setChecked(checked);
}

bool TaskAllocationPanel::isConstraintChecked(int index) const
{
    if (index >= 0 && index < m_constraintChecks.size())
        return m_constraintChecks[index]->isChecked();
    return false;
}

int TaskAllocationPanel::constraintCount() const
{
    return m_constraintRows.size();
}


// ═══════════════════════════════════════════════════════════
// 求解指标 API 实现
// ═══════════════════════════════════════════════════════════
int TaskAllocationPanel::addMetric(const QString &label, const QString &value,
                                    const QString &unit, const QString &tag,
                                    const QString &color)
{
    int id = m_metricCards.size();
    QFrame *card = createMetricCard(label, value, unit, tag, color);
    m_metricCards.append(card);

    if (m_metricGrid) {
        // 每 3 个指标换行（3 列网格）
        int row = id / 3;
        int col = id % 3;
        m_metricGrid->addWidget(card, row, col);
    }
    return id;
}

void TaskAllocationPanel::updateMetric(int index, const QString &value,
                                        const QString &tag, const QString &color)
{
    if (index < 0 || index >= m_metricCards.size()) return;

    // 更新数值文本和颜色
    if (index < m_metricValues.size() && m_metricValues[index]) {
        m_metricValues[index]->setText(value);
        if (!color.isEmpty())
            m_metricValues[index]->setStyleSheet(
                QString("color: %1; font-size: 20px; font-weight: bold; background: transparent; border: none; font-family: 'Consolas', monospace;").arg(color));
    }
    // 更新标记文本
    if (!tag.isEmpty() && index < m_metricTags.size() && m_metricTags[index]) {
        m_metricTags[index]->setText(tag);
    }
    // 更新单位颜色
    if (!color.isEmpty() && index < m_metricUnits.size() && m_metricUnits[index]) {
        m_metricUnits[index]->setStyleSheet(
            QString("color: %1; font-size: 11px; background: transparent; border: none; font-family: 'Consolas', monospace;").arg(color));
    }
}

void TaskAllocationPanel::removeMetric(int index)
{
    if (index < 0 || index >= m_metricCards.size()) return;
    QFrame *card = m_metricCards.takeAt(index);
    m_metricValues.takeAt(index);
    m_metricUnits.takeAt(index);
    m_metricTags.takeAt(index);
    card->deleteLater();

    // 重新排布网格：清空后按新顺序逐项重新 addWidget
    if (m_metricGrid) {
        QLayoutItem *item;
        while ((item = m_metricGrid->takeAt(0)) != nullptr) {}
        for (int i = 0; i < m_metricCards.size(); ++i) {
            int r = i / 3;
            int c = i % 3;
            m_metricGrid->addWidget(m_metricCards[i], r, c);
        }
    }
}

void TaskAllocationPanel::clearMetrics()
{
    for (QFrame *card : m_metricCards) card->deleteLater();
    m_metricCards.clear();
    m_metricValues.clear();
    m_metricUnits.clear();
    m_metricTags.clear();
}

int TaskAllocationPanel::metricCount() const
{
    return m_metricCards.size();
}


// ═══════════════════════════════════════════════════════════
// 候选方案 API 实现
// ═══════════════════════════════════════════════════════════
int TaskAllocationPanel::addAltPlan(const QString &name, const QString &fval,
                                     const QString &sorties, const QString &risk,
                                     bool isCurrent)
{
    int id = m_altPlanRows.size();
    QFrame *row = createAltPlanRow(name, fval, sorties, risk, isCurrent);
    m_altPlanRows.append(row);

    if (m_altPlanLayout)
        m_altPlanLayout->addWidget(row);
    return id;
}

void TaskAllocationPanel::removeAltPlan(int index)
{
    if (index < 0 || index >= m_altPlanRows.size()) return;
    QFrame *row = m_altPlanRows.takeAt(index);
    row->deleteLater();
}

void TaskAllocationPanel::clearAltPlans()
{
    for (QFrame *row : m_altPlanRows) row->deleteLater();
    m_altPlanRows.clear();
}

int TaskAllocationPanel::altPlanCount() const
{
    return m_altPlanRows.size();
}


// ═══════════════════════════════════════════════════════════
// 编队分配方案 API 实现
// ═══════════════════════════════════════════════════════════
int TaskAllocationPanel::addAllocGroup(const QString &target, const QString &name,
                                        const QString &priority, const QString &tot,
                                        const QList<UavSpec> &uavs,
                                        const QString &coordDesc)
{
    int id = m_allocFrames.size();
    QFrame *frame = createAllocGroupFrame(target, name, priority, tot, uavs, coordDesc);
    m_allocFrames.append(frame);

    if (m_allocScrollLayout) {
        m_allocScrollLayout->addWidget(frame);
    }
    return id;
}

void TaskAllocationPanel::removeAllocGroup(int index)
{
    if (index < 0 || index >= m_allocFrames.size()) return;
    QFrame *frame = m_allocFrames.takeAt(index);
    frame->deleteLater();
}

void TaskAllocationPanel::clearAllocGroups()
{
    for (QFrame *frame : m_allocFrames) frame->deleteLater();
    m_allocFrames.clear();
}

int TaskAllocationPanel::allocGroupCount() const
{
    return m_allocFrames.size();
}
