//
// Created by Administrator on 2026/5/6.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_ADDOREDITTASKPLAN_H
#define RZSIM_ANTI_RADIATION_UAV_ADDOREDITTASKPLAN_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class AddOrEditTaskplan; }
QT_END_NAMESPACE

class AddOrEditTaskplan : public QWidget {
Q_OBJECT

public:
    explicit AddOrEditTaskplan(QWidget *parent = nullptr);

    ~AddOrEditTaskplan() override;

private:
    Ui::AddOrEditTaskplan *ui;
};


#endif //RZSIM_ANTI_RADIATION_UAV_ADDOREDITTASKPLAN_H
