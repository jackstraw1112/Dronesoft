#include "SetAreaTargetEditDialog.h"
#include "../TaskPlanningTypeConvert.h"
#include "ui_SetAreaTargetEditDialog.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidgetItem>

SetAreaTargetEditDialog::SetAreaTargetEditDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SetAreaTargetEditDialog)
{
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
    AreaTargetInfo info;
    info.targetId = ui->leTargetId->text().trimmed();
    info.name = ui->leTargetName->text().trimmed();
    info.areaType = areaGeometryTypeFromChinese(ui->cmbAreaType->currentText());
    info.centerLatitude = ui->dsbCenterLatitude->value();
    info.centerLongitude = ui->dsbCenterLongitude->value();
    info.radiusKm = ui->dsbRadiusKm->value();
    info.expectedEmitters = ui->leExpectedEmitters->text().trimmed();
    info.searchStrategy = searchStrategyTypeFromChinese(ui->cmbSearchStrategy->currentText());
    info.priority = priorityFromChinese(ui->cmbPriority->currentText());

    info.vertices.clear();
    for (int row = 0; row < ui->tblVertices->rowCount(); ++row)
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
    ui->leTargetId->setText(info.targetId);
    ui->leTargetName->setText(info.name);
    ui->cmbAreaType->setCurrentText(areaGeometryTypeToChinese(info.areaType));
    ui->dsbCenterLatitude->setValue(info.centerLatitude);
    ui->dsbCenterLongitude->setValue(info.centerLongitude);
    ui->dsbRadiusKm->setValue(info.radiusKm);
    ui->leExpectedEmitters->setText(info.expectedEmitters);
    ui->cmbSearchStrategy->setCurrentText(searchStrategyTypeToChinese(info.searchStrategy));
    ui->cmbPriority->setCurrentText(priorityToChinese(info.priority));

    ui->tblVertices->setRowCount(0);
    for (int i = 0; i < info.vertices.size(); ++i)
    {
        ui->tblVertices->insertRow(i);
        ui->tblVertices->setItem(i, 0, new QTableWidgetItem(QString::number(info.vertices[i].latitude, 'f', 6)));
        ui->tblVertices->setItem(i, 1, new QTableWidgetItem(QString::number(info.vertices[i].longitude, 'f', 6)));
    }
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::setDialogTitle(const QString &title)
{
    setWindowTitle(title);
}

void SetAreaTargetEditDialog::setPickedVertices(const QList<GeoPoint> &vertices)
{
    ui->tblVertices->setRowCount(0);
    for (int i = 0; i < vertices.size(); ++i)
    {
        ui->tblVertices->insertRow(i);
        ui->tblVertices->setItem(i, 0, new QTableWidgetItem(QString::number(vertices[i].latitude, 'f', 6)));
        ui->tblVertices->setItem(i, 1, new QTableWidgetItem(QString::number(vertices[i].longitude, 'f', 6)));
    }
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::appendPickedVertex(const GeoPoint &point)
{
    const int row = ui->tblVertices->rowCount();
    ui->tblVertices->insertRow(row);
    ui->tblVertices->setItem(row, 0, new QTableWidgetItem(QString::number(point.latitude, 'f', 6)));
    ui->tblVertices->setItem(row, 1, new QTableWidgetItem(QString::number(point.longitude, 'f', 6)));
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onAcceptClicked()
{
    if (ui->leTargetId->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入区域目标编号。"));
        return;
    }
    if (ui->leTargetName->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入区域名称。"));
        return;
    }
    if (areaGeometryTypeFromChinese(ui->cmbAreaType->currentText()) == AreaGeometryType::Polygon)
    {
        int validCount = 0;
        for (int row = 0; row < ui->tblVertices->rowCount(); ++row)
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
    accept();
}

void SetAreaTargetEditDialog::onMapPickClicked()
{
    emit requestMapPick();
}

void SetAreaTargetEditDialog::onAddVertexClicked()
{
    const int row = ui->tblVertices->rowCount();
    ui->tblVertices->insertRow(row);
    ui->tblVertices->setItem(row, 0, new QTableWidgetItem());
    ui->tblVertices->setItem(row, 1, new QTableWidgetItem());
    ui->tblVertices->setCurrentCell(row, 0);
    ui->tblVertices->editItem(ui->tblVertices->item(row, 0));
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onEditVertexClicked()
{
    const int row = ui->tblVertices->currentRow();
    if (row < 0)
    {
        return;
    }
    if (!ui->tblVertices->item(row, 0))
    {
        ui->tblVertices->setItem(row, 0, new QTableWidgetItem());
    }
    ui->tblVertices->editItem(ui->tblVertices->item(row, 0));
}

void SetAreaTargetEditDialog::onDeleteVertexClicked()
{
    const int row = ui->tblVertices->currentRow();
    if (row < 0)
    {
        return;
    }
    ui->tblVertices->removeRow(row);
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::onAreaTypeChanged(int index)
{
    Q_UNUSED(index);
    const bool isPolygon = (areaGeometryTypeFromChinese(ui->cmbAreaType->currentText()) == AreaGeometryType::Polygon);
    ui->gpbPolygonVertices->setEnabled(isPolygon);
}

void SetAreaTargetEditDialog::updateVertexActionBtnState()
{
    const bool hasSelection = ui->tblVertices->currentRow() >= 0;
    ui->btnEditVertex->setEnabled(hasSelection);
    ui->btnDeleteVertex->setEnabled(hasSelection);
}

void SetAreaTargetEditDialog::initObject()
{
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
    ui->cmbPriority->addItems(QStringList()
                              << QStringLiteral("P1 · 紧急")
                              << QStringLiteral("P2 · 高")
                              << QStringLiteral("P3 · 一般"));

    ui->tblVertices->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tblVertices->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tblVertices->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tblVertices->verticalHeader()->setVisible(false);

    ui->dsbCenterLatitude->setRange(-90.0, 90.0);
    ui->dsbCenterLongitude->setRange(-180.0, 180.0);

    onAreaTypeChanged(ui->cmbAreaType->currentIndex());
    updateVertexActionBtnState();
}

void SetAreaTargetEditDialog::initConnect()
{
    connect(ui->btnConfirm, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onAcceptClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &SetAreaTargetEditDialog::reject);
    connect(ui->btn_Close, &QPushButton::clicked, this, &SetAreaTargetEditDialog::reject);
    connect(ui->cmbAreaType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SetAreaTargetEditDialog::onAreaTypeChanged);
    connect(ui->btnMapPick, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onMapPickClicked);
    connect(ui->btnAddVertex, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onAddVertexClicked);
    connect(ui->btnEditVertex, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onEditVertexClicked);
    connect(ui->btnDeleteVertex, &QPushButton::clicked, this, &SetAreaTargetEditDialog::onDeleteVertexClicked);
    connect(ui->tblVertices, &QTableWidget::itemSelectionChanged,
            this, &SetAreaTargetEditDialog::updateVertexActionBtnState);
}

bool SetAreaTargetEditDialog::tryParseVertexRow(int row, GeoPoint &point) const
{
    if (row < 0 || row >= ui->tblVertices->rowCount())
    {
        return false;
    }

    const QTableWidgetItem *latItem = ui->tblVertices->item(row, 0);
    const QTableWidgetItem *lonItem = ui->tblVertices->item(row, 1);
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

    point.latitude = lat;
    point.longitude = lon;
    return true;
}
