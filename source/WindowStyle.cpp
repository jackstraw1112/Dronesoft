#include "WindowStyle.h"
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

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

    widget->setAttribute(Qt::WA_Hover, true);
    widget->installEventFilter(this);
}

bool WindowStyle::eventFilter(QObject *obj, QEvent *event)
{
    QWidget *widget = qobject_cast<QWidget *>(obj);
    if (!widget)
        return QObject::eventFilter(obj, event);

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
    {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton)
        {
            m_dragging = true;
            m_dragStartPos = me->globalPos() - widget->window()->frameGeometry().topLeft();
            return true;
        }
        break;
    }
    case QEvent::MouseMove:
    {
        if (m_dragging)
        {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            widget->window()->move(me->globalPos() - m_dragStartPos);
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease:
    {
        if (m_dragging)
        {
            m_dragging = false;
            return true;
        }
        break;
    }
    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}
