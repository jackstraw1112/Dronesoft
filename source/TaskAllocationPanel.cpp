//
// Created by Administrator on 2026/5/6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_TaskAllocationPanel.h" resolved

#include "TaskAllocationPanel.h"
#include "ui_TaskAllocationPanel.h"


TaskAllocationPanel::TaskAllocationPanel(QWidget *parent) :
        QWidget(parent), ui(new Ui::TaskAllocationPanel) {
    ui->setupUi(this);
}

TaskAllocationPanel::~TaskAllocationPanel() {
    delete ui;
}
