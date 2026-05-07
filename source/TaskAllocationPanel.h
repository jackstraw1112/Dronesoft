//
// Created by Administrator on 2026/5/6.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_TASKALLOCATIONPANEL_H
#define RZSIM_ANTI_RADIATION_UAV_TASKALLOCATIONPANEL_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class TaskAllocationPanel; }
QT_END_NAMESPACE

class TaskAllocationPanel : public QWidget {
Q_OBJECT

public:
    explicit TaskAllocationPanel(QWidget *parent = nullptr);

    ~TaskAllocationPanel() override;

private:
    Ui::TaskAllocationPanel *ui;
};


#endif //RZSIM_ANTI_RADIATION_UAV_TASKALLOCATIONPANEL_H
