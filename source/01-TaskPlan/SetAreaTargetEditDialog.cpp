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

    info.targetId = ui->targetIdEdit->text().trimmed();
    info.name = ui->targetNameEdit->text().trimmed();
    info.areaType = areaGeometryTypeFromChinese(ui->areaTypeCombo->currentText());
    info.centerLatitude = ui->centerLatitudeSpb->value();
    info.centerLongitude = ui->centerLongitudeSpb->value();
    info.radiusKm = ui->radiusKmSpb->value();
    info.expectedEmitters = ui->expectedEmittersEdit->text().trimmed();
    info.searchStrategy = searchStrategyTypeFromChinese(ui->searchStrategyCombo->currentText());
    info.priority = priorityFromChinese(ui->priorityCombo->currentText());

    // 顶点列表由表格驱动：逐行解析有效经纬度，忽略无效/空行。
    info.vertices.clear();
    for (int row = 0; row < ui->verticesTbw->rowCount(); ++row)
    {
        GeoPoint point;
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
    ui->targetIdEdit->setText(info.targetId);
    ui->targetNameEdit->setText(info.name);
    ui->areaTypeCombo->setCurrentText(areaGeometryTypeToChinese(info.areaType));
    ui->centerLatitudeSpb->setValue(info.centerLatitude);
    ui->centerLongitudeSpb->setValue(info.centerLongitude);
    ui->radiusKmSpb->setValue(info.radiusKm);
    ui->expectedEmittersEdit->setText(info.expectedEmitters);
    ui->searchStrategyCombo->setCurrentText(searchStrategyTypeToChinese(info.searchStrategy));
    ui->priorityCombo->setCurrentText(priorityToChinese(info.priority));

    // 先清空再按顺序插入，保证显示与缓存一致。
    ui->verticesTbw->setRowCount(0);
    for (int i = 0; i < info.vertices.size(); ++i)
    {
        ui->verticesTbw->insertRow(i);
        ui->verticesTbw->setItem(i, 0, new QTableWidgetItem(QString::number(info.vertices[i].latitude, 'f', 6)));
        ui->verticesTbw->setItem(i, 1, new QTableWidgetItem(QString::number(info.vertices[i].longitude, 'f', 6)));
    }
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::setDialogTitle(const QString &title)
{
    // 外部可在新增/编辑场景下动态设置标题文案。
    setWindowTitle(title);
}

void SetAreaTargetEditDialog::setPickedVertices(const QList<GeoPoint> &vertices)
{
    // 地图一次性回填多点：完全覆盖当前表格。
    ui->verticesTbw->setRowCount(0);
    for (int i = 0; i < vertices.size(); ++i)
    {
        ui->verticesTbw->insertRow(i);
        ui->verticesTbw->setItem(i, 0, new QTableWidgetItem(QString::number(vertices[i].latitude, 'f', 6)));
        ui->verticesTbw->setItem(i, 1, new QTableWidgetItem(QString::number(vertices[i].longitude, 'f', 6)));
    }
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::appendPickedVertex(const GeoPoint &point)
{
    // 地图增量回填单点：追加到末尾，不影响已有行顺序。
    const int row = ui->verticesTbw->rowCount();
    ui->verticesTbw->insertRow(row);
    ui->verticesTbw->setItem(row, 0, new QTableWidgetItem(QString::number(point.latitude, 'f', 6)));
    ui->verticesTbw->setItem(row, 1, new QTableWidgetItem(QString::number(point.longitude, 'f', 6)));
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onAcceptClicked()
{
    // 基础必填校验：编号和名称不能为空。
    if (ui->targetIdEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入区域目标编号。"));
        return;
    }

    if (ui->targetNameEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入区域名称。"));
        return;
    }

    // 多边形区域额外约束：每行坐标有效且至少 3 个点。
    if (areaGeometryTypeFromChinese(ui->areaTypeCombo->currentText()) == AreaGeometryType::Polygon)
    {
        int validCount = 0;
        for (int row = 0; row < ui->verticesTbw->rowCount(); ++row)
        {
            GeoPoint point;
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
    const int row = ui->verticesTbw->rowCount();
    ui->verticesTbw->insertRow(row);
    ui->verticesTbw->setItem(row, 0, new QTableWidgetItem());
    ui->verticesTbw->setItem(row, 1, new QTableWidgetItem());
    ui->verticesTbw->setCurrentCell(row, 0);
    ui->verticesTbw->editItem(ui->verticesTbw->item(row, 0));
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onEditVertexClicked()
{
    // 编辑当前选中行。若单元格对象不存在则先补一个空 item。
    const int row = ui->verticesTbw->currentRow();
    if (row < 0)
    {
        return;
    }
    if (!ui->verticesTbw->item(row, 0))
    {
        ui->verticesTbw->setItem(row, 0, new QTableWidgetItem());
    }
    ui->verticesTbw->editItem(ui->verticesTbw->item(row, 0));
}

void SetAreaTargetEditDialog::onDeleteVertexClicked()
{
    // 删除当前选中行（仅操作表格，数据在确认时再统一读取）。
    const int row = ui->verticesTbw->currentRow();
    if (row < 0)
    {
        return;
    }
    ui->verticesTbw->removeRow(row);
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onAreaTypeChanged(int index)
{
    Q_UNUSED(index);
    // 仅多边形需要“区域坐标”组；其他类型可禁用该组避免误填。
    const bool isPolygon = (areaGeometryTypeFromChinese(ui->areaTypeCombo->currentText()) == AreaGeometryType::Polygon);
    ui->polygonVerticesGb->setEnabled(isPolygon);
}

void SetAreaTargetEditDialog::updateVertexActionBtnState()
{
    // 编辑/删除按钮依赖当前是否有选中行。
    const bool hasSelection = ui->verticesTbw->currentRow() >= 0;
    ui->editVertexBtn->setEnabled(hasSelection);
    ui->deleteVertexBtn->setEnabled(hasSelection);
}

void SetAreaTargetEditDialog::initObject()
{
    // 对话框静态配置：标题、下拉项、表格行为、经纬度范围。
    setWindowTitle(QStringLiteral("新增区域目标"));

    ui->areaTypeCombo->addItems(QStringList()
                              << QStringLiteral("圆形")
                              << QStringLiteral("矩形")
                              << QStringLiteral("多边形")
                              << QStringLiteral("走廊"));

    ui->searchStrategyCombo->addItems(QStringList()
                                    << QStringLiteral("螺旋扫描")
                                    << QStringLiteral("平行线扫描")
                                    << QStringLiteral("扇形扫描")
                                    << QStringLiteral("盘旋搜索"));

    ui->priorityCombo->addItems(QStringList()
                              << QStringLiteral("P1 · 紧急")
                              << QStringLiteral("P2 · 高")
                              << QStringLiteral("P3 · 一般"));

    // 顶点表按“行”操作，便于增删改。
    ui->verticesTbw->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->verticesTbw->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->verticesTbw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->verticesTbw->verticalHeader()->setVisible(false);

    // 中心经纬度取值范围限制。
    ui->centerLatitudeSpb->setRange(-90.0, 90.0);
    ui->centerLongitudeSpb->setRange(-180.0, 180.0);

    // 根据默认区域类型初始化控件可用状态。
    onAreaTypeChanged(ui->areaTypeCombo->currentIndex());
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::initConnect()
{
    // 底部确认/取消与右上角关闭按钮。
    connect(ui->confirmBtn, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onAcceptClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &SetAreaTargetEditDialog::reject);
    connect(ui->closeBtn, &QPushButton::clicked, this, &SetAreaTargetEditDialog::reject);

    // 区域类型切换时，联动“区域坐标组”可用状态。
    connect(ui->areaTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SetAreaTargetEditDialog::onAreaTypeChanged);

    // 顶点组相关操作（地图拾取、增删改、选中态）。
    connect(ui->mapPickBtn, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onMapPickClicked);
    connect(ui->addVertexBtn, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onAddVertexClicked);
    connect(ui->editVertexBtn, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onEditVertexClicked);
    connect(ui->deleteVertexBtn, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onDeleteVertexClicked);
    connect(ui->verticesTbw, &QTableWidget::itemSelectionChanged,
            this, &SetAreaTargetEditDialog::updateVertexActionBtnState);
}

bool SetAreaTargetEditDialog::tryParseVertexRow(int row, GeoPoint &point) const
{
    // 防御式解析：行号、单元格存在性、数值格式、经纬度范围均需合法。
    if (row < 0 || row >= ui->verticesTbw->rowCount())
    {
        return false;
    }

    const QTableWidgetItem *latItem = ui->verticesTbw->item(row, 0);
    const QTableWidgetItem *lonItem = ui->verticesTbw->item(row, 1);
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
