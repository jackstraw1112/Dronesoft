//
// Created by Administrator on 2026/5/7.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H
#define RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H

#include <QWidget>
#include <QTableWidget>
#include <QCheckBox>
#include "TaskPlanningData.h"


QT_BEGIN_NAMESPACE
namespace Ui { class ParameterLoading; }
QT_END_NAMESPACE

class ParameterLoading : public QWidget {
Q_OBJECT

public:
    explicit ParameterLoading(QWidget *parent = nullptr);

    ~ParameterLoading() override;

    void applyTechStyle();
    void setupDroneTable();
    void updateStatusBar();
    void setAssignmentData(const QList<UavAssignment> &assignments,
                           const QList<UavResource> &resourcePool = QList<UavResource>());

public slots:
    void restoreDefaults();

private slots:
    void onVerifyClicked();
    void onDefaultClicked();
    void onBatchUploadClicked();
    void onRefreshTableClicked();
    void onSelectAllChanged(int state);
    void onDroneCheckChanged();

private:
    void connectSignals();
    void populateSampleData();

    Ui::ParameterLoading *ui;
    QList<QCheckBox*> m_droneCheckBoxes;
    int m_selectedCount = 0;
    QList<UavAssignment> m_assignments;
    QList<UavResource> m_resourcePool;
};


#endif //RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H
