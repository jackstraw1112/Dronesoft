//
// Created by Administrator on 2026/5/6.
//
// 兵力需求计算面板 - 实现文件

// You may need to build the project (run Qt uic code generator) to get "ui_ForceRequirementPanel.h" resolved

#include "ForceRequirementPanel.h"
#include "ui_ForceRequirementPanel.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QDebug>
#include <QtMath>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>
#include <QFile>
#include <QCoreApplication>
#include <QDir>

// 构造函数：初始化UI、设置雷达目标数据、应用样式、连接信号槽
ForceRequirementPanel::ForceRequirementPanel(QWidget *parent) : QFrame(parent), ui(new Ui::ForceRequirementPanel)
{
    ui->setupUi(this);

    // 初始化雷达目标选择区（scroll area + pill 网格布局 + 布局约束）
    initTargetScrollArea();

    // 初始显示点目标模型页面
    ui->stackedWidget->setCurrentIndex(0);
    m_currentTargetId = "";
    m_currentTargetType = "";
    m_currentDamageLevel = 0.9; // 默认高毁伤要求

    // ⚠️ 先应用全局样式，再创建 pill（确保 pill 的 inline 样式不会被父级 QSS 覆盖）
    applyTechStyle();

    // 填充默认计算目标
    setupDefaultTargets();
    if (m_targetPills.size() > 0) selectTarget(0);

    setupConnections();          // 初始化信号槽连接
    setupPtInputConnections();   // 初始化点目标输入参数信号槽
    setupArInputConnections();   // 初始化区域目标输入参数信号槽
}

// 析构函数
ForceRequirementPanel::~ForceRequirementPanel()
{
    delete ui;
}

// 初始化雷达目标选择区：配置 scroll area + pill 网格布局 + 布局约束
void ForceRequirementPanel::initTargetScrollArea()
{
    m_scrollArea = ui->scrollArea_Target;
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidgetResizable(true);

    m_scrollWidget = ui->scrollAreaWidgetContents;
    m_targetGrid = ui->gridLayout;
    m_targetGrid->setSpacing(6);
    m_targetGrid->setContentsMargins(6, 6, 6, 6);

    QVBoxLayout *outer = qobject_cast<QVBoxLayout*>(layout());
    if (outer) {
        outer->setStretch(2, 1);
        outer->setSizeConstraint(QLayout::SetMinimumSize);
    }
    QVBoxLayout *widgetLayout = qobject_cast<QVBoxLayout*>(ui->widget->layout());
    if (widgetLayout) {
        widgetLayout->setSizeConstraint(QLayout::SetMinimumSize);
    }
}

// 初始化信号槽连接：绑定单独计算、全部计算按钮
void ForceRequirementPanel::setupConnections()
{
    connect(ui->btn_AngleCalculate, &QPushButton::clicked, this, &ForceRequirementPanel::onSingleCalculate);
    connect(ui->btn_AllCalculate, &QPushButton::clicked, this, &ForceRequirementPanel::onAllCalculate);
}

// 初始化点目标输入参数信号槽：毁伤要求按钮、单弹毁伤概率、备份架数变化时重新计算
void ForceRequirementPanel::setupPtInputConnections()
{
    ui->btn_DamageLevel_Low->setCheckable(true);
    ui->btn_DamageLevel_Mid->setCheckable(true);
    ui->btn_DamageLevel_High->setCheckable(true);

    connect(ui->btn_DamageLevel_Low, &QPushButton::clicked, this, [this]() {
        m_currentDamageLevel = 0.7;
        ui->btn_DamageLevel_Low->setChecked(true);
        ui->btn_DamageLevel_Mid->setChecked(false);
        ui->btn_DamageLevel_High->setChecked(false);
        updatePtCalculation();
    });
    connect(ui->btn_DamageLevel_Mid, &QPushButton::clicked, this, [this]() {
        m_currentDamageLevel = 0.8;
        ui->btn_DamageLevel_Low->setChecked(false);
        ui->btn_DamageLevel_Mid->setChecked(true);
        ui->btn_DamageLevel_High->setChecked(false);
        updatePtCalculation();
    });
    connect(ui->btn_DamageLevel_High, &QPushButton::clicked, this, [this]() {
        m_currentDamageLevel = 0.9;
        ui->btn_DamageLevel_Low->setChecked(false);
        ui->btn_DamageLevel_Mid->setChecked(false);
        ui->btn_DamageLevel_High->setChecked(true);
        updatePtCalculation();
    });
    connect(ui->spinBox_SingleShotProb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        updatePtCalculation();
    });
    connect(ui->spinBox_BackupCount, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        updatePtCalculation();
    });
}

// 初始化区域目标输入参数信号槽：区域面积、估计目标数、每目标弹数、备份架数变化时重新计算
void ForceRequirementPanel::setupArInputConnections()
{
    connect(ui->spinBox_Area, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        updateArCalculation();
    });
    connect(ui->spinBox_EstTargetCount, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        updateArCalculation();
    });
    connect(ui->spinBox_ShotsPerTarget, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        updateArCalculation();
    });
    connect(ui->spinBox_AreaBackupCount, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        updateArCalculation();
    });
}

// 更新点目标计算结果：根据毁伤要求P̄和单弹毁伤概率Pk计算所需弹数n = ⌈log(1-P̄)/log(1-Pk)⌉，再加备份架数
void ForceRequirementPanel::updatePtCalculation()
{
    double P_bar = m_currentDamageLevel;
    double Pk = ui->spinBox_SingleShotProb->value();
    int backup = ui->spinBox_BackupCount->value();

    double logNumerator = qLn(1.0 - P_bar);
    double logDenominator = qLn(1.0 - Pk);
    double quotient = logNumerator / logDenominator;
    int n = static_cast<int>(qCeil(quotient));
    int total = n + backup;

    PtCalcData data;
    data.damageLevel = P_bar;
    data.Pk = Pk;
    data.n = n;
    data.backup = backup;
    data.total = total;
    m_ptResults[m_currentTargetId] = data;

    updateSummaryRow(m_currentTargetId);
    emit forceResultsChanged();
}

// 更新区域目标计算结果：N搜 = ⌈面积/25⌉，N打 = ⌈估计目标数×每目标弹数⌉，总 = MAX(N搜,N打) + 备份
void ForceRequirementPanel::updateArCalculation()
{
    const double coveragePerUav = 25.0; // 单机覆盖面积 KM²
    int area = ui->spinBox_Area->value();
    double estTargets = ui->spinBox_EstTargetCount->value();
    int shotsPerTarget = ui->spinBox_ShotsPerTarget->value();
    int backup = ui->spinBox_AreaBackupCount->value();

    int N_search = static_cast<int>(qCeil(area / coveragePerUav));
    int N_strike = static_cast<int>(qCeil(estTargets * shotsPerTarget));
    int N_max = qMax(N_search, N_strike);
    int total = N_max + backup;

    ArCalcData data;
    data.area = area;
    data.estTargets = estTargets;
    data.shotsPerTarget = shotsPerTarget;
    data.backup = backup;
    data.N_search = N_search;
    data.N_strike = N_strike;
    data.total = total;
    m_arResults[m_currentTargetId] = data;

    updateSummaryRow(m_currentTargetId);
    emit forceResultsChanged();
}

void ForceRequirementPanel::applyTechStyle()
{
    const QString fileName = QStringLiteral("force_requirement_panel.qss");
    const QString relPath = QStringLiteral("theme/") + fileName;

    const QString exeDir = QCoreApplication::applicationDirPath();
    QStringList candidates;
    candidates << (exeDir + QStringLiteral("/") + relPath);
    candidates << (exeDir + QStringLiteral("/../") + relPath);
    candidates << (exeDir + QStringLiteral("/../../") + relPath);
    candidates << QDir::cleanPath(exeDir + QStringLiteral("/../source/theme/") + fileName);

    for (const QString &path : candidates) {
        QFile f(path);
        if (!f.exists())
            continue;
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QString qss = QString::fromUtf8(f.readAll());
        if (!qss.trimmed().isEmpty()) {
            setStyleSheet(qss);
            return;
        }
    }

    qWarning() << "[ForceRequirementPanel] Failed to load theme:" << relPath
               << "(tried near executable/source)";
}

// 单独计算：仅计算当前选中的目标，刷新汇总表格
void ForceRequirementPanel::onSingleCalculate()
{
    if (m_currentTargetId.isEmpty()) return;

    if (m_currentTargetType == "PT")
        updatePtCalculation();
    else if (m_currentTargetType == "AR")
        updateArCalculation();
}

// 全部计算：遍历所有目标，依次计算并刷新汇总表格
void ForceRequirementPanel::onAllCalculate()
{
    // 记住当前选中的目标 ID，计算完毕后恢复
    const QString prevTargetId = m_currentTargetId;
    const QString prevTargetType = m_currentTargetType;

    for (int i = 0; i < m_targetIds.size(); ++i)
    {
        const QString &id = m_targetIds[i];
        const QString &type = m_targetTypes[i];

        // 切换到该目标并恢复其输入参数
        saveCurrentInput();
        restoreInput(id, type);
        m_currentTargetId = id;
        m_currentTargetType = type;

        if (type == "PT")
            updatePtCalculation();
        else
            updateArCalculation();
    }

    // 恢复之前选中目标的输入参数
    if (!prevTargetId.isEmpty())
    {
        saveCurrentInput();
        restoreInput(prevTargetId, prevTargetType);
        m_currentTargetId = prevTargetId;
        m_currentTargetType = prevTargetType;

        // 刷新 UI 页面
        if (prevTargetType == "PT")
        {
            ui->stackedWidget->setCurrentIndex(0);
            ui->label_PointModel->setText(QString("点目标模型 · POINT MODEL  [ %1 ]").arg(prevTargetId));
        }
        else
        {
            ui->stackedWidget->setCurrentIndex(1);
            ui->label_PointModel->setText(QString("区域目标模型 · AREA MODEL  [ %1 ]").arg(prevTargetId));
        }
    }
}

// 更新汇总表格中指定目标的行
void ForceRequirementPanel::updateSummaryRow(const QString &targetId)
{
    int idx = m_targetIds.indexOf(targetId);
    if (idx < 0) return;

    QTableWidget *table = ui->tableWidget_Count;
    QString typeDisplay = (m_targetTypes[idx] == "PT") ? "点目标" : "区域目标";

    // 查找该目标是否已有行
    int row = -1;
    for (int r = 0; r < table->rowCount(); ++r)
    {
        QTableWidgetItem *item = table->item(r, 0);
        if (item && item->data(Qt::UserRole).toString() == targetId)
        {
            row = r;
            break;
        }
    }

    auto makeItem = [](const QString &text) {
        auto *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        return item;
    };

    // ── 设置表格列头（仅在首次填充时） ──
    if (table->columnCount() == 0)
    {
        QStringList headers;
        headers << "目标" << "类型" << "毁伤要求" << "建议架次" << "备份架次" << "总架数" << "匹配机型";
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setAlternatingRowColors(true);
    }

    if (row < 0)
    {
        row = table->rowCount();
        table->insertRow(row);
        table->setRowHeight(row, 32);
    }

    // 填充数据
    if (m_targetTypes[idx] == "PT")
    {
        PtCalcData d = m_ptResults.value(targetId);
        table->setItem(row, 0, makeItem(QString("%1 %2").arg(targetId).arg(m_targetNames[idx])));
        table->setItem(row, 1, makeItem(typeDisplay));
        table->setItem(row, 2, makeItem(QString("%1").arg(d.damageLevel, 0, 'f', 2)));
        table->setItem(row, 3, makeItem(QString("%1架").arg(d.n)));
        table->setItem(row, 4, makeItem(QString("%1架").arg(d.backup)));
        table->setItem(row, 5, makeItem(QString("%1架").arg(d.total)));
        table->setItem(row, 6, makeItem(QString("反辐射无人机")));
    }
    else
    {
        ArCalcData d = m_arResults.value(targetId);
        table->setItem(row, 0, makeItem(QString("%1 %2").arg(targetId).arg(m_targetNames[idx])));
        table->setItem(row, 1, makeItem(typeDisplay));
        table->setItem(row, 2, makeItem(QString("N/A")));
        table->setItem(row, 3, makeItem(QString("%1架").arg(d.N_search)));
        table->setItem(row, 4, makeItem(QString("%1架").arg(d.backup)));
        table->setItem(row, 5, makeItem(QString("%1架").arg(d.total)));
        table->setItem(row, 6, makeItem(QString("反辐射无人机")));
    }

    // 在 item 中存 targetId 用于查找
    if (table->item(row, 0))
        table->item(row, 0)->setData(Qt::UserRole, targetId);
}

// ─────────────────────────────────────────────
// QFrame 点击事件过滤
// ─────────────────────────────────────────────
// 捕获目标 pill 的鼠标点击，选中对应索引的目标并取消其他选中
bool ForceRequirementPanel::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QPushButton *pill = qobject_cast<QPushButton*>(watched);
        if (pill) {
            int index = m_targetPills.indexOf(pill);
            if (index >= 0) {
                selectTarget(index);
                return true;
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

// ─────────────────────────────────────────────
// 目标 pill 工厂方法
// ─────────────────────────────────────────────

// 更新单个 pill 的选中/未选中样式
static void updatePillStyle(QPushButton *pill, bool selected)
{
    if (selected) {
        pill->setStyleSheet(
            "QPushButton {"
            "   background-color: #0d2466;"
            "   border: 1px solid #00b4ff;"
            "   border-radius: 6px;"
            "   color: #e0e8f0;"
            "   font-size: 12px;"
            "   text-align: left;"
            "   padding: 0px 10px;"
            "}"
        );
    } else {
        pill->setStyleSheet(
            "QPushButton {"
            "   background-color: #0b1124;"
            "   border: 1px solid #1a3a6a;"
            "   border-radius: 6px;"
            "   color: #e0e8f0;"
            "   font-size: 12px;"
            "   text-align: left;"
            "   padding: 0px 10px;"
            "}"
        );
    }
    pill->setProperty("selected", selected);
}

// 创建一个可点击的 pill 控件，使用 QPushButton 确保渲染可见
// pill 包含：类型色标 + 目标ID + 目标名称
QPushButton *ForceRequirementPanel::createTargetPill(const QString &id, const QString &name, const QString &type)
{
    QPushButton *pill = new QPushButton(m_scrollWidget);
    pill->setFixedHeight(36);
    pill->setCursor(Qt::PointingHandCursor);
    pill->setFlat(true);
    pill->setProperty("selected", false);
    pill->setProperty("targetPill", true);   // 标记为 target pill，跳过通用按钮样式

    // 使用 QHBoxLayout 内嵌文字
    QHBoxLayout *layout = new QHBoxLayout(pill);
    layout->setSpacing(6);
    layout->setContentsMargins(10, 4, 10, 4);

    // 类型色标（PT=青色, AR=琥珀色）
    QLabel *badge = new QLabel(pill);
    badge->setFixedSize(8, 8);
    badge->setStyleSheet(QString("background-color: %1; border-radius: 4px; border: none;")
                         .arg(type == "PT" ? "#00e5ff" : "#ffb300"));

    // 目标ID
    QLabel *idLabel = new QLabel(id, pill);
    idLabel->setStyleSheet("color: #e0e8f0; font-weight: bold; font-size: 12px; "
                           "font-family: 'Consolas'; background: transparent; border: none;");

    // 目标名称
    QLabel *nameLabel = new QLabel(name, pill);
    nameLabel->setStyleSheet("color: #7a8ba8; font-size: 11px; "
                             "font-family: 'Microsoft YaHei'; background: transparent; border: none;");

    layout->addWidget(badge);
    layout->addWidget(idLabel);
    layout->addWidget(nameLabel);
    layout->addStretch();

    updatePillStyle(pill, false);
    return pill;
}

// 更新网格行伸缩，使所有 pill 保持顶部对齐
static void updateGridRowStretch(QGridLayout *grid, int pillCount, int columns)
{
    int lastRow = (pillCount - 1) / columns;
    if (lastRow < 0) lastRow = 0;
    for (int r = 0; r <= lastRow + 1; ++r)
        grid->setRowStretch(r, 0);
    grid->setRowStretch(lastRow + 1, 1);
}

// ─────────────────────────────────────────────
// 填充默认计算目标
// ─────────────────────────────────────────────
void ForceRequirementPanel::setupDefaultTargets()
{
    addTarget("PT-01", "东郊制导雷达", "PT");
    addTarget("PT-02", "东郊火控雷达", "PT");
    addTarget("PT-03", "北山警戒雷达", "PT");
    addTarget("AR-01", "集北防空集群区", "AR");
    addTarget("AR-02", "店坪雷达走廊", "AR");
    addTarget("PT-04", "南山预警雷达", "PT");

    // 默认选中第一个
    selectTarget(0);

    // 顶部对齐
    updateGridRowStretch(m_targetGrid, m_targetPills.size(), 4);
    updateTargetGroupHeight();
}

// ─────────────────────────────────────────────
// 根据 pill 数量直接调整目标容器高度，驱动父布局重新分配空间
// ─────────────────────────────────────────────
void ForceRequirementPanel::updateTargetGroupHeight()
{
    int count = m_targetPills.size();
    int rows = (count + 3) / 4;
    if (rows < 1) rows = 1;

    const int kPillHeight = 36;
    const int kGridSpacing = 6;
    const int kGridMargin = 6;

    int contentHeight = rows * kPillHeight + (rows - 1) * kGridSpacing + 2 * kGridMargin;

    // 三级同时设 minimumHeight，确保布局链每层都能感知高度变化
    m_scrollWidget->setMinimumHeight(contentHeight);
    m_scrollArea->setMinimumHeight(contentHeight);
    // widget 容器内含标题(~30px) + 间距(~6px) + scrollArea
    ui->widget->setMinimumHeight(36 + contentHeight);

    // 强制整个布局链重新计算：panel → QStackedWidget → widget_Center → MissionPlanner
    layout()->invalidate();
    this->setMinimumHeight(layout()->minimumSize().height());
    this->updateGeometry();

    QWidget *p = parentWidget();
    int levels = 0;
    while (p && levels < 6) {
        p->updateGeometry();
        if (p->layout()) {
            p->layout()->invalidate();
        }
        p = p->parentWidget();
        ++levels;
    }
}

// ─────────────────────────────────────────────
// 目标选择 API 实现
// ─────────────────────────────────────────────

// 添加一个计算目标 pill
int ForceRequirementPanel::addTarget(const QString &id, const QString &name, const QString &type)
{
    if (!m_targetGrid) return -1;

    int index = m_targetPills.size();
    QPushButton *pill = createTargetPill(id, name, type);

    int row = index / 4;
    int col = index % 4;
    m_targetGrid->addWidget(pill, row, col);

    // 保存到成员列表
    m_targetPills.append(pill);
    m_targetIds.append(id);
    m_targetNames.append(name);
    m_targetTypes.append(type);

    // 初始化输入参数记录（使用 UI 默认值）
    if (type == "PT") {
        PtInputData pt;
        pt.damageLevel = 0.9;
        pt.singleShotProb = 0.95;
        pt.backupCount = 1;
        m_ptInputs[id] = pt;
    } else {
        ArInputData ar;
        ar.area = 100;
        ar.estTargetCount = 3.0;
        ar.shotsPerTarget = 2;
        ar.backupCount = 2;
        m_arInputs[id] = ar;
    }

    // 点击事件：通过 eventFilter 捕获 mouse press
    pill->installEventFilter(this);

    // 更新顶部对齐
    updateGridRowStretch(m_targetGrid, m_targetPills.size(), 4);
    updateTargetGroupHeight();

    return index;
}

// 移除指定索引的计算目标
void ForceRequirementPanel::removeTarget(int index)
{
    if (index < 0 || index >= m_targetPills.size()) return;

    // 从网格布局中移除
    QPushButton *pill = m_targetPills[index];
    QString removedId = m_targetIds[index];
    m_targetGrid->removeWidget(pill);
    delete pill;
    m_targetPills.removeAt(index);
    m_targetIds.removeAt(index);
    m_targetNames.removeAt(index);
    m_targetTypes.removeAt(index);

    // 清理对应的输入参数和计算结果记录
    m_ptInputs.remove(removedId);
    m_arInputs.remove(removedId);
    m_ptResults.remove(removedId);
    m_arResults.remove(removedId);

    // 重新布局剩余 pill
    for (int i = 0; i < m_targetPills.size(); ++i) {
        int row = i / 4;
        int col = i % 4;
        m_targetGrid->addWidget(m_targetPills[i], row, col);
    }

    // 更新顶部对齐
    updateGridRowStretch(m_targetGrid, m_targetPills.size(), 4);
    updateTargetGroupHeight();

    // 更新选择状态
}

// 清空所有计算目标
void ForceRequirementPanel::clearTargets()
{
    for (QPushButton *pill : m_targetPills) {
        m_targetGrid->removeWidget(pill);
        delete pill;
    }
    m_targetPills.clear();
    m_targetIds.clear();
    m_targetNames.clear();
    m_targetTypes.clear();

    // 清理输入参数和计算结果记录
    m_ptInputs.clear();
    m_arInputs.clear();
    m_ptResults.clear();
    m_arResults.clear();

    // 更新顶部对齐
    updateGridRowStretch(m_targetGrid, 0, 4);
    updateTargetGroupHeight();
}

// 获取计算目标总数
int ForceRequirementPanel::targetCount() const
{
    return m_targetPills.size();
}

// 获取指定索引的目标 ID
QString ForceRequirementPanel::targetId(int index) const
{
    if (index < 0 || index >= m_targetIds.size()) return QString();
    return m_targetIds[index];
}

// 获取指定索引的目标名称
QString ForceRequirementPanel::targetName(int index) const
{
    if (index < 0 || index >= m_targetNames.size()) return QString();
    return m_targetNames[index];
}

// 获取指定索引的目标类型
QString ForceRequirementPanel::targetType(int index) const
{
    if (index < 0 || index >= m_targetTypes.size()) return QString();
    return m_targetTypes[index];
}

// 根据几何类型计算区域面积（平方千米），返回至少为 1
static int calcAreaFromGeometry(const AreaTargetInfo &ar);

// 从任务加载点目标和区域目标：清空旧 pill → 添加新 pill → 注入目标参数 → 全量自动计算
void ForceRequirementPanel::loadTaskTargets(const QList<PointTargetInfo> &pointTargets,
                                            const QList<AreaTargetInfo> &areaTargets)
{
    clearTargets();

    for (const PointTargetInfo &pt : pointTargets)
    {
        addTarget(pt.targetId, pt.name, QStringLiteral("PT"));
        PtInputData in;
        in.damageLevel = (pt.requiredPk > 0.0) ? pt.requiredPk : 0.9;
        in.singleShotProb = 0.95;
        in.backupCount = 1;
        m_ptInputs[pt.targetId] = in;
    }
    for (const AreaTargetInfo &ar : areaTargets)
    {
        addTarget(ar.targetId, ar.name, QStringLiteral("AR"));
        ArInputData in;
        in.area = calcAreaFromGeometry(ar);
        in.estTargetCount = 3.0;
        in.shotsPerTarget = 2;
        in.backupCount = 2;
        m_arInputs[ar.targetId] = in;
    }

    if (m_targetPills.isEmpty())
        return;

    setUpdatesEnabled(false);
    const int count = m_targetPills.size();
    for (int i = 0; i < count; ++i)
        selectTarget(i);
    setUpdatesEnabled(true);

    selectTarget(0);
}

// 根据几何类型计算区域面积（平方千米），返回至少为 1
static int calcAreaFromGeometry(const AreaTargetInfo &ar)
{
    switch (ar.areaType)
    {
    case AreaGeometryType::Circle:
        if (ar.radiusKm > 0.0)
            return qMax(1, static_cast<int>(3.14159265 * ar.radiusKm * ar.radiusKm));
        return 100;

    case AreaGeometryType::Rectangle:
        if (ar.vertices.size() >= 4)
        {
            double minLat = ar.vertices[0].latitude, maxLat = ar.vertices[0].latitude;
            double minLon = ar.vertices[0].longitude, maxLon = ar.vertices[0].longitude;
            for (const GeoPosition &v : ar.vertices)
            {
                if (v.latitude  < minLat) minLat = v.latitude;
                if (v.latitude  > maxLat) maxLat = v.latitude;
                if (v.longitude < minLon) minLon = v.longitude;
                if (v.longitude > maxLon) maxLon = v.longitude;
            }
            const double latSpan = (maxLat - minLat) * 111.32;
            const double lonSpan = (maxLon - minLon) * 111.32 * qCos(qDegreesToRadians((minLat + maxLat) * 0.5));
            return qMax(1, static_cast<int>(latSpan * lonSpan));
        }
        return 100;

    case AreaGeometryType::Polygon:
        if (ar.vertices.size() >= 3)
        {
            double area = 0.0;
            const int n = ar.vertices.size();
            for (int i = 0; i < n; ++i)
            {
                const int j = (i + 1) % n;
                const double xi = ar.vertices[i].longitude * 111.32 * qCos(qDegreesToRadians(ar.vertices[i].latitude));
                const double yi = ar.vertices[i].latitude * 111.32;
                const double xj = ar.vertices[j].longitude * 111.32 * qCos(qDegreesToRadians(ar.vertices[j].latitude));
                const double yj = ar.vertices[j].latitude * 111.32;
                area += xi * yj - xj * yi;
            }
            return qMax(1, static_cast<int>(qAbs(area) * 0.5));
        }
        return 100;

    case AreaGeometryType::Corridor:
        if (ar.vertices.size() >= 2)
        {
            const double lat1 = ar.vertices[0].latitude * 111.32;
            const double lon1 = ar.vertices[0].longitude * 111.32 * qCos(qDegreesToRadians(ar.vertices[0].latitude));
            const double lat2 = ar.vertices[1].latitude * 111.32;
            const double lon2 = ar.vertices[1].longitude * 111.32 * qCos(qDegreesToRadians(ar.vertices[1].latitude));
            const double lengthKm = qSqrt((lat2 - lat1) * (lat2 - lat1) + (lon2 - lon1) * (lon2 - lon1));
            const double widthKm = (ar.radiusKm > 0.0) ? ar.radiusKm * 2.0 : 10.0;
            return qMax(1, static_cast<int>(lengthKm * widthKm));
        }
        if (ar.radiusKm > 0.0)
            return qMax(1, static_cast<int>(3.14159265 * ar.radiusKm * ar.radiusKm));
        return 100;

    default:
        if (ar.radiusKm > 0.0)
            return qMax(1, static_cast<int>(3.14159265 * ar.radiusKm * ar.radiusKm));
        return 100;
    }
}

// 保存当前选中目标的输入参数到映射表
void ForceRequirementPanel::saveCurrentInput()
{
    if (m_currentTargetId.isEmpty()) return;

    if (m_currentTargetType == "PT") {
        PtInputData pt;
        pt.damageLevel = m_currentDamageLevel;
        pt.singleShotProb = ui->spinBox_SingleShotProb->value();
        pt.backupCount = ui->spinBox_BackupCount->value();
        m_ptInputs[m_currentTargetId] = pt;
    } else if (m_currentTargetType == "AR") {
        ArInputData ar;
        ar.area = ui->spinBox_Area->value();
        ar.estTargetCount = ui->spinBox_EstTargetCount->value();
        ar.shotsPerTarget = ui->spinBox_ShotsPerTarget->value();
        ar.backupCount = ui->spinBox_AreaBackupCount->value();
        m_arInputs[m_currentTargetId] = ar;
    }
}

// 从映射表恢复指定目标的输入参数到 UI 控件
void ForceRequirementPanel::restoreInput(const QString &id, const QString &type)
{
    if (type == "PT") {
        if (!m_ptInputs.contains(id)) return;
        const PtInputData &pt = m_ptInputs[id];
        m_currentDamageLevel = pt.damageLevel;
        ui->spinBox_SingleShotProb->setValue(pt.singleShotProb);
        ui->spinBox_BackupCount->setValue(pt.backupCount);
        // 同步毁伤要求按钮选中状态（取最近档位）
        ui->btn_DamageLevel_Low->setChecked(pt.damageLevel < 0.75);
        ui->btn_DamageLevel_Mid->setChecked(pt.damageLevel >= 0.75 && pt.damageLevel < 0.85);
        ui->btn_DamageLevel_High->setChecked(pt.damageLevel >= 0.85);
    } else if (type == "AR") {
        if (!m_arInputs.contains(id)) return;
        const ArInputData &ar = m_arInputs[id];
        ui->spinBox_Area->setValue(ar.area);
        ui->spinBox_EstTargetCount->setValue(ar.estTargetCount);
        ui->spinBox_ShotsPerTarget->setValue(ar.shotsPerTarget);
        ui->spinBox_AreaBackupCount->setValue(ar.backupCount);
    }
}

// 选中指定索引的计算目标
void ForceRequirementPanel::selectTarget(int index)
{
    if (index < 0 || index >= m_targetPills.size()) return;

    // 保存当前选中目标的输入参数
    saveCurrentInput();

    // 取消所有选中
    for (int i = 0; i < m_targetPills.size(); ++i) {
        updatePillStyle(m_targetPills[i], i == index);
    }

    // 更新当前目标信息
    m_currentTargetId = m_targetIds[index];
    m_currentTargetType = m_targetTypes[index];

    // 从记录中恢复该目标的输入参数
    restoreInput(m_currentTargetId, m_currentTargetType);

    // 切换对应模型页面并刷新计算结果
    if (m_currentTargetType == "PT") {
        ui->label_PointModel->setText(QString("点目标模型 · POINT MODEL  [ %1 ]").arg(m_currentTargetId));
        ui->stackedWidget->setCurrentIndex(0);
        updatePtCalculation();
    } else {
        ui->label_PointModel->setText(QString("区域目标模型 · AREA MODEL  [ %1 ]").arg(m_currentTargetId));
        ui->stackedWidget->setCurrentIndex(1);
        updateArCalculation();
    }
}

// 从外部恢复已保存的兵力计算结果：覆盖 m_ptResults/m_arResults，
// 若当前选中目标匹配则同步刷新 UI 计算按钮文字。
void ForceRequirementPanel::restoreResults(const QMap<QString, PtCalcData> &ptResults,
                                            const QMap<QString, ArCalcData> &arResults)
{
    for (auto it = ptResults.cbegin(); it != ptResults.cend(); ++it)
        m_ptResults[it.key()] = it.value();
    for (auto it = arResults.cbegin(); it != arResults.cend(); ++it)
        m_arResults[it.key()] = it.value();

    if (!m_currentTargetId.isEmpty()) {
        if (m_currentTargetType == "PT" && m_ptResults.contains(m_currentTargetId))
            updatePtCalculation();
        else if (m_currentTargetType == "AR" && m_arResults.contains(m_currentTargetId))
            updateArCalculation();
    }

    emit forceResultsChanged();
}
