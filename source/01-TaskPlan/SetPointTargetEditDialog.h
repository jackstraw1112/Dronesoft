#ifndef POINTTARGETEDITDIALOG_H
#define POINTTARGETEDITDIALOG_H

#include <QDialog>
#include "../TaskPlanningData.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class SetPointTargetEditDialog;
}
QT_END_NAMESPACE

class SetPointTargetEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetPointTargetEditDialog(QWidget *parent = nullptr);
    ~SetPointTargetEditDialog() override;

    PointTargetInfo pointTargetInfo() const;
    void setTargetNameOptions(const QStringList &targetNames);
    void setPointTargetInfo(const PointTargetInfo &info);
    void setDialogTitle(const QString &title);

private slots:
    void onAcceptClicked();

private:
    void initObject();
    void initConnect();

    Ui::SetPointTargetEditDialog *ui;
};

#endif // POINTTARGETEDITDIALOG_H
