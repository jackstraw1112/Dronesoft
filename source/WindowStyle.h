#ifndef WINDOWSTYLE_H
#define WINDOWSTYLE_H

#include <QObject>
#include <QWidget>

class WindowStyle : public QObject
{
    Q_OBJECT
public:
    explicit WindowStyle(QObject *parent = nullptr);
    ~WindowStyle();

    void activateOn(QWidget *widget);
};

#endif
