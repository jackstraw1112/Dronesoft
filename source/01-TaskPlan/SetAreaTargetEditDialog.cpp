#include "SetAreaTargetEditDialog.h"
#include "TaskPlanningTypeConvert.h"
#include "ui_SetAreaTargetEditDialog.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidgetItem>

SetAreaTargetEditDialog::SetAreaTargetEditDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SetAreaTargetEditDialog)
{
    // 初始化顺序固定为：构建界面 -> 初始化控件属性 -> 连接信号槽。
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    initObject();
    initConnect();
}

SetAreaTargetEditDialog::~SetAreaTargetEditDialog()
{
    delete ui;
}

AreaTargetInfo SetAreaTargetEditDialog::areaTargetInfo() const
{
    // 从界面采集并组装业务对象，作为对话框输出结果。
    AreaTargetInfo info;

    info.name = ui->leTargetName->text().trimmed();
    info.areaType = areaGeometryTypeFromChinese(ui->cmbAreaType->currentText());
    info.centerLatitude = ui->spbCenterLatitude->value();
    info.centerLongitude = ui->spbCenterLongitude->value();
    info.radiusKm = ui->spbRadiusKm->value();
    info.expectedEmitters = ui->leExpectedEmitters->text().trimmed();
    info.searchStrategy = searchStrategyTypeFromChinese(ui->cmbSearchStrategy->currentText());
    info.priority = priorityFromChinese(ui->cmbPriority->currentText());

    // 顶点列表由表格驱动：逐行解析有效经纬度，忽略无效/空行。
    info.vertices.clear();
    for (int row = 0; row < ui->tbwVertices->rowCount(); ++row)
    {
        GeoPosition point;
        if (tryParseVertexRow(row, point))
        {
            info.vertices.append(point);
        }
    }
    return info;
}

void SetAreaTargetEditDialog::setAreaTargetInfo(const AreaTargetInfo &info)
{
    // 编辑模式回填：先填基础字段，再重建顶点表格。
    ui->leTargetName->setText(info.name);
    ui->cmbAreaType->setCurrentText(areaGeometryTypeToChinese(info.areaType));
    ui->spbCenterLatitude->setValue(info.centerLatitude);
    ui->spbCenterLongitude->setValue(info.centerLongitude);
    ui->spbRadiusKm->setValue(info.radiusKm);
    ui->leExpectedEmitters->setText(info.expectedEmitters);
    ui->cmbSearchStrategy->setCurrentText(searchStrategyTypeToChinese(info.searchStrategy));
    ui->cmbPriority->setCurrentText(priorityToChinese(info.priority));

    // 先清空再按顺序插入，保证显示与缓存一致。
    ui->tbwVertices->setRowCount(0);
    for (int i = 0; i < info.vertices.size(); ++i)
    {
        ui->tbwVertices->insertRow(i);
        ui->tbwVertices->setItem(i, 0, new QTableWidgetItem(QString::number(info.vertices[i].latitude, 'f', 6)));
        ui->tbwVertices->setItem(i, 1, new QTableWidgetItem(QString::number(info.vertices[i].longitude, 'f', 6)));
    }
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::setDialogTitle(const QString &title)
{
    // 外部可在新增/编辑场景下动态设置标题文案。
    setWindowTitle(title);
}

void SetAreaTargetEditDialog::setPickedVertices(const QList<GeoPosition> &vertices)
{
    // 地图一次性回填多点：完全覆盖当前表格。
    ui->tbwVertices->setRowCount(0);
    for (int i = 0; i < vertices.size(); ++i)
    {
        ui->tbwVertices->insertRow(i);
        ui->tbwVertices->setItem(i, 0, new QTableWidgetItem(QString::number(vertices[i].latitude, 'f', 6)));
        ui->tbwVertices->setItem(i, 1, new QTableWidgetItem(QString::number(vertices[i].longitude, 'f', 6)));
    }
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::appendPickedVertex(const GeoPosition &point)
{
    // 地图增量回填单点：追加到末尾，不影响已有行顺序。
    const int row = ui->tbwVertices->rowCount();
    ui->tbwVertices->insertRow(row);
    ui->tbwVertices->setItem(row, 0, new QTableWidgetItem(QString::number(point.latitude, 'f', 6)));
    ui->tbwVertices->setItem(row, 1, new QTableWidgetItem(QString::number(point.longitude, 'f', 6)));
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onAcceptClicked()
{
    // 基础必填校验：名称不能为空。
    if (ui->leTargetName->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入区域名称。"));
        return;
    }

    // 多边形区域额外约束：每行坐标有效且至少 3 个点。
    if (areaGeometryTypeFromChinese(ui->cmbAreaType->currentText()) == AreaGeometryType::Polygon)
    {
        int validCount = 0;
        for (int row = 0; row < ui->tbwVertices->rowCount(); ++row)
        {
            GeoPosition point;
            if (!tryParseVertexRow(row, point))
            {
                QMessageBox::warning(this, QStringLiteral("提示"),
                                     QStringLiteral("第%1行坐标无效，请输入有效经纬度。").arg(row + 1));
                return;
            }
            ++validCount;
        }
        if (validCount < 3)
        {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("多边形区域至少需要三个坐标点。"));
            return;
        }
    }
    // 所有校验通过后关闭对话框并返回 Accepted。
    accept();
}

void SetAreaTargetEditDialog::onMapPickClicked()
{
    // 仅发信号，不耦合地图实现；由外层窗口决定如何拾取并回填。
    emit requestMapPick();
}

void SetAreaTargetEditDialog::onAddVertexClicked()
{
    // 添加“空白顶点行”，并将编辑焦点落到新行第一列。
    const int row = ui->tbwVertices->rowCount();
    ui->tbwVertices->insertRow(row);
    ui->tbwVertices->setItem(row, 0, new QTableWidgetItem());
    ui->tbwVertices->setItem(row, 1, new QTableWidgetItem());
    ui->tbwVertices->setCurrentCell(row, 0);
    ui->tbwVertices->editItem(ui->tbwVertices->item(row, 0));
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onEditVertexClicked()
{
    // 编辑当前选中行。若单元格对象不存在则先补一个空 item。
    const int row = ui->tbwVertices->currentRow();
    if (row < 0)
    {
        return;
    }
    if (!ui->tbwVertices->item(row, 0))
    {
        ui->tbwVertices->setItem(row, 0, new QTableWidgetItem());
    }
    ui->tbwVertices->editItem(ui->tbwVertices->item(row, 0));
}

void SetAreaTargetEditDialog::onDeleteVertexClicked()
{
    // 删除当前选中行（仅操作表格，数据在确认时再统一读取）。
    const int row = ui->tbwVertices->currentRow();
    if (row < 0)
    {
        return;
    }
    ui->tbwVertices->removeRow(row);
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onAreaTypeChanged(int index)
{
    Q_UNUSED(index);
    // 仅多边形需要“区域坐标”组；其他类型可禁用该组避免误填。
    const bool isPolygon = (areaGeometryTypeFromChinese(ui->cmbAreaType->currentText()) == AreaGeometryType::Polygon);
    ui->gbPolygonVertices->setEnabled(isPolygon);
}

void SetAreaTargetEditDialog::updateVertexActionBtnState()
{
    // 编辑/删除按钮依赖当前是否有选中行。
    const bool hasSelection = ui->tbwVertices->currentRow() >= 0;
    ui->btnEditVertex->setEnabled(hasSelection);
    ui->btnDeleteVertex->setEnabled(hasSelection);
}

void SetAreaTargetEditDialog::initObject()
{
    // 对话框静态配置：标题、下拉项、表格行为、经纬度范围。
    setWindowTitle(QStringLiteral("新增区域目标"));

    ui->cmbAreaType->addItems(QStringList()
                              << QStringLiteral("圆形")
                              << QStringLiteral("矩形")
                              << QStringLiteral("多边形")
                              << QStringLiteral("走廊"));

    ui->cmbSearchStrategy->addItems(QStringList()
                                    << QStringLiteral("螺旋扫描")
                                    << QStringLiteral("平行线扫描")
                                    << QStringLiteral("扇形扫描")
                                    << QStringLiteral("盘旋搜索"));

    // 优先级：决定目标打击顺序——紧急目标优先分配兵力与航路资源
    ui->cmbPriority->addItems(QStringList()
                              << QStringLiteral("P1 · 优先打击")
                              << QStringLiteral("P2 · 正常处理")
                              << QStringLiteral("P3 · 可延后"));
    // 表单左右两列等比拉伸，窗口横向变化时两侧同步扩展。
    ui->gridFormLy->setColumnStretch(0, 1);
    ui->gridFormLy->setColumnStretch(1, 1);

    // 字段标签统一美化：增强“标签在上、输入在下”的层次感。
    const QString fieldLabelStyle = QStringLiteral(
            "QLabel { color: #00b4ff; font-size: 10px; font-weight: 600; padding: 0 0 2px 2px; }");
    ui->targetNameLbl->setStyleSheet(fieldLabelStyle);
    ui->areaTypeLbl->setStyleSheet(fieldLabelStyle);
    ui->searchStrategyLbl->setStyleSheet(fieldLabelStyle);
    ui->radiusLbl->setStyleSheet(fieldLabelStyle);
    ui->lblPriority->setStyleSheet(fieldLabelStyle);
    ui->centerLatitudeLbl->setStyleSheet(fieldLabelStyle);
    ui->centerLongitudeLbl->setStyleSheet(fieldLabelStyle);
    ui->expectedEmittersLbl->setStyleSheet(fieldLabelStyle);

    ui->leTargetName->setPlaceholderText(QStringLiteral("请输入区域名称"));
    ui->leExpectedEmitters->setPlaceholderText(QStringLiteral("例如：预警雷达, 火控雷达"));

    // 顶点表按“行”操作，便于增删改。
    ui->tbwVertices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tbwVertices->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tbwVertices->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tbwVertices->verticalHeader()->setVisible(false);

    // 中心经纬度取值范围限制。
    ui->spbCenterLatitude->setRange(-90.0, 90.0);
    ui->spbCenterLongitude->setRange(-180.0, 180.0);

    // 根据默认区域类型初始化控件可用状态。
    onAreaTypeChanged(ui->cmbAreaType->currentIndex());
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::initConnect()
{
    // 底部确认/取消按钮。
    connect(ui->btnConfirm, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onAcceptClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &SetAreaTargetEditDialog::reject);

    // 区域类型切换时，联动“区域坐标组”可用状态。
    connect(ui->cmbAreaType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SetAreaTargetEditDialog::onAreaTypeChanged);

    // 顶点组相关操作（地图拾取、增删改、选中态）。
    connect(ui->btnMapPick, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onMapPickClicked);
    connect(ui->btnAddVertex, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onAddVertexClicked);
    connect(ui->btnEditVertex, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onEditVertexClicked);
    connect(ui->btnDeleteVertex, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onDeleteVertexClicked);
    connect(ui->tbwVertices, &QTableWidget::itemSelectionChanged,
            this, &SetAreaTargetEditDialog::updateVertexActionBtnState);
}

bool SetAreaTargetEditDialog::tryParseVertexRow(int row, GeoPosition &point) const
{
    // 防御式解析：行号、单元格存在性、数值格式、经纬度范围均需合法。
    if (row < 0 || row >= ui->tbwVertices->rowCount())
    {
        return false;
    }

    const QTableWidgetItem *latItem = ui->tbwVertices->item(row, 0);
    const QTableWidgetItem *lonItem = ui->tbwVertices->item(row, 1);
    if (!latItem || !lonItem)
    {
        return false;
    }

    bool okLat = false;
    bool okLon = false;
    const double lat = latItem->text().trimmed().toDouble(&okLat);
    const double lon = lonItem->text().trimmed().toDouble(&okLon);
    if (!okLat || !okLon)
    {
        return false;
    }
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
    {
        return false;
    }

    // 解析成功后写回输出参数。
    point.latitude = lat;
    point.longitude = lon;
    return true;
}
