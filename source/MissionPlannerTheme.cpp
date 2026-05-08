#include "MissionPlannerTheme.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStringList>

namespace
{
QString themeRelativePath()
{
    return QStringLiteral("theme/") + MissionPlannerTheme::themeFileBaseName();
}

QString executableDir()
{
    return QCoreApplication::applicationDirPath();
}
} // namespace

QString MissionPlannerTheme::themeFileBaseName()
{
    return QStringLiteral("mission_planner_theme.qss");
}

QString MissionPlannerTheme::loadThemeStylesheet()
{
    const QString rel = themeRelativePath();

    const QString exeDir = executableDir();

    QStringList candidates;
    candidates << (exeDir + QStringLiteral("/") + rel);
    candidates << (exeDir + QStringLiteral("/../") + rel);
    candidates << (exeDir + QStringLiteral("/../../") + rel);

    const QString srcRoot = QStringLiteral("%1/../source/theme/%2")
                                    .arg(exeDir, themeFileBaseName());
    candidates << QDir::cleanPath(srcRoot);

    for (const QString &path : candidates)
    {
        QFile f(path);
        if (!f.exists())
            continue;

        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        const QString qss = QString::fromUtf8(f.readAll());
        if (!qss.trimmed().isEmpty())
            return qss;
    }

    return {};
}

bool MissionPlannerTheme::applyToApplication(QApplication *app)
{
    if (!app)
        return false;

    const QString qss = loadThemeStylesheet();
    if (qss.isEmpty())
    {
        qWarning() << "[MissionPlannerTheme] Failed to load theme:" << themeRelativePath()
                   << "(tried near executable/source)";
        return false;
    }

    app->setStyleSheet(qss);
    return true;
}
