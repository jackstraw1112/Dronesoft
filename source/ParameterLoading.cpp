//
// Created by Administrator on 2026/5/7.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ParameterLoading.h" resolved

#include "ParameterLoading.h"
#include "ui_ParameterLoading.h"


ParameterLoading::ParameterLoading(QWidget *parent) :
        QWidget(parent), ui(new Ui::ParameterLoading) {
    ui->setupUi(this);
}

ParameterLoading::~ParameterLoading() {
    delete ui;
}
