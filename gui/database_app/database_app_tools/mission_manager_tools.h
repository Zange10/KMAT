#ifndef KSP_MISSION_MANAGER_TOOLS_H
#define KSP_MISSION_MANAGER_TOOLS_H

#include "database/mission_database.h"
#include <gtk/gtk.h>

void init_mission_manager(GtkBuilder *builder);
void switch_to_mission_manager(struct Mission_DB mission);

#endif //KSP_MISSION_MANAGER_TOOLS_H