//
// Created by Administrator on 2026/5/7.
//

// You may need to build the project (run Qt uic code generator) to get "ui_RoutePlanning.h" resolved

#include "RoutePlanning.h"
#include "ui_RoutePlanning.h"


RoutePlanning::RoutePlanning(QWidget *parent) :QWidget(parent), ui(new Ui::RoutePlanning)
{
    ui->setupUi(this);
}

RoutePlanning::~RoutePlanning()
{
    delete ui;
}
