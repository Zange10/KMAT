#ifndef KSP_MISSION_DB_TOOLS_H
#define KSP_MISSION_DB_TOOLS_H

#include "database/mission_database.h"
#include <gtk/gtk.h>


void init_mission_db_tools(GtkBuilder *builder);
int get_active_combobox_id(GtkComboBox *combo_box);
void set_active_combobox_id(GtkComboBox *combobox, int target_id);
struct MissionProgram_DB get_program_from_id(struct MissionProgram_DB *programs, int num_programs, int program_id);
struct Mission_DB get_mission_from_id(struct Mission_DB *missions, int num_missions, int mission_id);
void set_gtk_widget_class_by_mission_success(GtkWidget *widget, struct Mission_DB mission);
void update_program_dropdown(GtkComboBox *combo_box, int show_init_all);
void update_mission_launcher_dropdown(GtkComboBox *combo_box);
void switch_to_mission_database_page();
void switch_to_mission_manager_page();
void add_mission_id_to_info_show_list(int **show_mission_info, int *num_mission_info, int mission_id);
void remove_mission_id_from_info_show_list(int *show_mission_info, int num_mission_info, int mission_id);
int is_mission_id_on_info_show_list(const int *show_mission_info, int num_mission_info, int mission_id);



#endif //KSP_MISSION_DB_TOOLS_H
