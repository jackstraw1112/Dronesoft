// 点目标编辑弹窗头文件
// 提供点目标基础字段编辑、目标名称下拉选择与参数校验能力。
// 该对话框既用于“新增点目标”，也用于“编辑点目标”。

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
    // 构造/析构
    explicit SetPointTargetEditDialog(QWidget *parent = nullptr);
    ~SetPointTargetEditDialog() override;

    // 导出当前界面数据为 PointTargetInfo。
    PointTargetInfo pointTargetInfo() const;
    // 设置目标名称下拉选项（由外层业务传入）。
    void setTargetNameOptions(const QStringList &targetNames);
    // 将已有点目标数据回填到界面（编辑模式）。
    void setPointTargetInfo(const PointTargetInfo &info);
    // 设置弹窗标题（新增/编辑文案切换）。
    void setDialogTitle(const QString &title);

private slots:
    // 点击“确认”后的校验与提交流程。
    void onAcceptClicked();

private:
    // 初始化控件属性与默认值。
    void initObject();
    // 绑定信号与槽函数。
    void initConnect();

    // UI 对象
    Ui::SetPointTargetEditDialog *ui;
};

#endif // POINTTARGETEDITDIALOG_H
