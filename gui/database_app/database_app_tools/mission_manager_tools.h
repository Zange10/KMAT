#ifndef KSP_MISSION_MANAGER_TOOLS_H
#define KSP_MISSION_MANAGER_TOOLS_H

#include "database/mission_database.h"
#include <gtk/gtk.h>

void init_mission_manager(GtkBuilder *builder);
void switch_to_mission_manager(struct Mission_DB mission);



// Handler -------------------------------------
G_MODULE_EXPORT void on_enter_new_program();
G_MODULE_EXPORT void on_add_new_program();
G_MODULE_EXPORT void on_mman_back();
G_MODULE_EXPORT void on_newupdate_mission();
G_MODULE_EXPORT void on_mman_add_objective(GtkWidget *button, gpointer data);
G_MODULE_EXPORT void on_mman_remove_objective(GtkWidget *button, gpointer data);
G_MODULE_EXPORT void on_mman_add_event(GtkWidget *button, gpointer data);
G_MODULE_EXPORT void on_mman_remove_event(GtkWidget *button, gpointer data);
G_MODULE_EXPORT void on_mman_change_event_time_type(GtkWidget *combo_box, gpointer data);

#endif //KSP_MISSION_MANAGER_TOOLS_H