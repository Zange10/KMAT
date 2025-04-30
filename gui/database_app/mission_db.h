#ifndef KSP_MISSION_DB_H
#define KSP_MISSION_DB_H

#include <gtk/gtk.h>

void init_mission_db(GtkBuilder *builder);
void update_db_box();
void close_mission_db();


// Handler ---------------------------------------------
G_MODULE_EXPORT void on_show_missions(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_reset_mission_filter(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_new_mission();
G_MODULE_EXPORT void on_showhide_mission_objectives(GtkWidget *button, gpointer data);
G_MODULE_EXPORT void on_showhide_mission_events(GtkWidget *button, gpointer data);
G_MODULE_EXPORT void on_edit_mission(GtkWidget *button, gpointer data);
G_MODULE_EXPORT void on_update_mission_init_event(GtkWidget *button, gpointer data);


#endif //KSP_MISSION_DB_H
