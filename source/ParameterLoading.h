//
// Created by Administrator on 2026/5/7.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H
#define RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class ParameterLoading; }
QT_END_NAMESPACE

class ParameterLoading : public QWidget {
Q_OBJECT

public:
    explicit ParameterLoading(QWidget *parent = nullptr);

    ~ParameterLoading() override;

private:
    Ui::ParameterLoading *ui;
};


#endif //RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H
