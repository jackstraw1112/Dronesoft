//
// Created by Administrator on 2026/5/6.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_FORCEREQUIREMENTPANEL_H
#define RZSIM_ANTI_RADIATION_UAV_FORCEREQUIREMENTPANEL_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class ForceRequirementPanel; }
QT_END_NAMESPACE

class ForceRequirementPanel : public QWidget {
Q_OBJECT

public:
    explicit ForceRequirementPanel(QWidget *parent = nullptr);

    ~ForceRequirementPanel() override;

private:
    Ui::ForceRequirementPanel *ui;
};


#endif //RZSIM_ANTI_RADIATION_UAV_FORCEREQUIREMENTPANEL_H
