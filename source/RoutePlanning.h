//
// Created by Administrator on 2026/5/7.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H
#define RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class RoutePlanning; }
QT_END_NAMESPACE

class RoutePlanning : public QWidget {
Q_OBJECT

public:
    explicit RoutePlanning(QWidget *parent = nullptr);

    ~RoutePlanning() override;

private:
    Ui::RoutePlanning *ui;
};


#endif //RZSIM_ANTI_RADIATION_UAV_ROUTEPLANNING_H
