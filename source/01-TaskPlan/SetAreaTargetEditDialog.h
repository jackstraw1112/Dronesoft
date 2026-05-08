// 区域目标编辑弹窗头文件
// 提供区域目标基础字段编辑、多边形顶点维护（增删改）以及地图拾取回填接口。
// 该对话框既用于“新增区域目标”，也用于“编辑区域目标”。

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
    // 构造/析构
    explicit SetAreaTargetEditDialog(QWidget *parent = nullptr);
    ~SetAreaTargetEditDialog() override;

    // 导出当前界面数据为 AreaTargetInfo。
    AreaTargetInfo areaTargetInfo() const;
    // 将已有区域目标数据回填到界面（编辑模式）。
    void setAreaTargetInfo(const AreaTargetInfo &info);
    // 设置弹窗标题（新增/编辑文案切换）。
    void setDialogTitle(const QString &title);
    // 批量回填地图拾取顶点（覆盖模式）。
    void setPickedVertices(const QList<GeoPosition> &vertices);
    // 追加单个拾取顶点（增量模式）。
    void appendPickedVertex(const GeoPosition &point);

signals:
    // 请求外部地图模块进行拾取
    void requestMapPick();

private slots:
    // 点击“确认”后的校验与提交流程。
    void onAcceptClicked();
    // 点击“地图拾取”，向外发请求信号。
    void onMapPickClicked();
    // 顶点表：添加空白行。
    void onAddVertexClicked();
    // 顶点表：编辑当前选中行。
    void onEditVertexClicked();
    // 顶点表：删除当前选中行。
    void onDeleteVertexClicked();
    // 区域类型变化时联动顶点组可用状态。
    void onAreaTypeChanged(int index);
    // 刷新顶点编辑/删除按钮可用状态。
    void updateVertexActionBtnState();

private:
    // 初始化控件属性与默认值。
    void initObject();
    // 绑定信号与槽函数。
    void initConnect();
    // 解析并校验指定表格行的经纬度数据。
    bool tryParseVertexRow(int row, GeoPosition &point) const;

    // UI 对象
    Ui::SetAreaTargetEditDialog *ui;
};

#endif // AREATARGETEDITDIALOG_H
