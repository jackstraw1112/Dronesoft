#ifndef AREATARGETEDITDIALOG_H
#define AREATARGETEDITDIALOG_H

#include <QDialog>
#include "../TaskPlanningData.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class SetAreaTargetEditDialog;
}
QT_END_NAMESPACE

class SetAreaTargetEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetAreaTargetEditDialog(QWidget *parent = nullptr);
    ~SetAreaTargetEditDialog() override;

    AreaTargetInfo areaTargetInfo() const;
    void setAreaTargetInfo(const AreaTargetInfo &info);
    void setDialogTitle(const QString &title);
    void setPickedVertices(const QList<GeoPoint> &vertices);
    void appendPickedVertex(const GeoPoint &point);

signals:
    // 请求外部地图模块进行拾取
    void requestMapPick();

private slots:
    void onAcceptClicked();
    void onMapPickClicked();
    void onAddVertexClicked();
    void onEditVertexClicked();
    void onDeleteVertexClicked();
    void onAreaTypeChanged(int index);
    void updateVertexActionBtnState();

private:
    void initObject();
    void initConnect();
    bool tryParseVertexRow(int row, GeoPoint &point) const;

    Ui::SetAreaTargetEditDialog *ui;
};

#endif // AREATARGETEDITDIALOG_H
