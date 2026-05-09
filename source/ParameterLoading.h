//
// Created by Administrator on 2026/5/7.
//

#ifndef RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H
#define RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H

#include <QWidget>
#include <QTableWidget>
#include <QCheckBox>


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
};


#endif //RZSIM_ANTI_RADIATION_UAV_PARAMETERLOADING_H
