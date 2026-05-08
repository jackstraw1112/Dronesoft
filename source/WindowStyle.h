#ifndef WINDOWSTYLE_H
#define WINDOWSTYLE_H

#include <QEvent>
#include <QObject>
#include <QPoint>
#include <QWidget>

class WindowStyle : public QObject
{
    Q_OBJECT
public:
    explicit WindowStyle(QObject *parent = nullptr);
    ~WindowStyle() override;

    void activateOn(QWidget *widget);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

#endif
