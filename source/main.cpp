#include <QApplication>
#include <QDebug>
#include "MissionPlanner.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MissionPlanner *window = MissionPlanner::GetInstance();
    window->show();

    return app.exec();
}
