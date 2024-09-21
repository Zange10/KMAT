#ifndef KSP_MISSION_DB_TOOLS_H
#define KSP_MISSION_DB_TOOLS_H

#include "database/mission_database.h"
#include <gtk/gtk.h>

int get_active_combobox_id(GtkComboBox *combo_box);
void set_active_combobox_id(GtkComboBox *combobox, int target_id);
struct MissionProgram_DB get_program_from_id(struct MissionProgram_DB *programs, int num_programs, int program_id);
struct Mission_DB get_mission_from_id(struct Mission_DB *missions, int num_missions, int mission_id);
void set_gtk_widget_class_by_mission_success(GtkWidget *widget, struct Mission_DB mission);

#endif //KSP_MISSION_DB_TOOLS_H
