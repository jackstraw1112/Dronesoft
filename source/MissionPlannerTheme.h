// Unified Qt StyleSheet loader for MissionPlanner desktop UI.
//
// All visual styling is authored in source/theme/mission_planner_theme.qss and copied next to the
// executable at build time (${OUTPUT_DIR}/theme/mission_planner_theme.qss).

#ifndef MISSIONPLANNERTHEME_H
#define MISSIONPLANNERTHEME_H

#include <QString>

class QApplication;

namespace MissionPlannerTheme
{
QString themeFileBaseName();

// Reads the authoritative QSS (first readable candidate path wins) or returns empty.
QString loadThemeStylesheet();

// Applies the theme to the entire application (recommended for dialogs too).
bool applyToApplication(QApplication *app);

} // namespace MissionPlannerTheme

#endif // MISSIONPLANNERTHEME_H
