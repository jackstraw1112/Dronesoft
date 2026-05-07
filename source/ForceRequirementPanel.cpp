//
// Created by Administrator on 2026/5/6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ForceRequirementPanel.h" resolved

#include "ForceRequirementPanel.h"
#include "ui_ForceRequirementPanel.h"


ForceRequirementPanel::ForceRequirementPanel(QWidget *parent) :
        QWidget(parent), ui(new Ui::ForceRequirementPanel) {
    ui->setupUi(this);
}

ForceRequirementPanel::~ForceRequirementPanel() {
    delete ui;
}
