// 任务项组件头文件
// 任务项组件用于展示单个任务的基本信息，并响应选中与点击事件

#ifndef TASKITEMWIDGET_H
#define TASKITEMWIDGET_H

#include <QFrame>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class TaskItemWidget;
}
QT_END_NAMESPACE

class QMouseEvent;
class QLabel;

class TaskItemWidget : public QFrame
{
    Q_OBJECT

public:
    explicit TaskItemWidget(QWidget *parent = nullptr);
    ~TaskItemWidget();

    // 设置任务编号（显示为青色等宽字体）
    void setTaskId(QString taskId);
    // 设置任务名称（显示为加粗文本）
    void setTaskName(QString taskName);
    // 设置任务状态：根据状态类型显示不同颜色标签
    void setTaskStatus(QString status);
    // 设置目标数量（显示格式：◉ X 目标）
    void setTargetCount(int count);
    // 设置威胁等级（显示格式：⚠ 威胁：高/中/低）
    void setThreatLevel(const QString &level);
    // 设置任务时间（显示格式：⏱ 开始 ~ 结束）
    void setTime(QString time);

    QString getTaskId()
    {
        return m_taskId;
    }
    void setSelected(bool selected);

signals:
    void clicked(QString taskId);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    // 初始化参数
    void initParams();
    // 初始化对象
    void initObject();
    // 关联信号与槽函数
    void initConnect();
    void loadStyleSheet();
    void refreshStyle();

private:
    Ui::TaskItemWidget *ui;
    QString m_taskId;
    QString m_status;
};

#endif // TASKITEMWIDGET_H
