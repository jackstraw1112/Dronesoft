#include "WindowStyle.h"
#include <QMouseEvent>
#include <QApplication>

WindowStyle::WindowStyle(QObject *parent)
    : QObject(parent)
{
}

WindowStyle::~WindowStyle()
{
}

void WindowStyle::activateOn(QWidget *widget)
{
    if (!widget)
        return;

    widget->setProperty("windowStyle", true);
}
