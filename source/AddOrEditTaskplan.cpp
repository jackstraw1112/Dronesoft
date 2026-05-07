//
// Created by Administrator on 2026/5/6.
//

// You may need to build the project (run Qt uic code generator) to get "ui_AddOrEditTaskplan.h" resolved

#include "AddOrEditTaskplan.h"
#include "ui_AddOrEditTaskplan.h"


AddOrEditTaskplan::AddOrEditTaskplan(QWidget *parent) :
        QWidget(parent), ui(new Ui::AddOrEditTaskplan) {
    ui->setupUi(this);
}

AddOrEditTaskplan::~AddOrEditTaskplan() {
    delete ui;
}
