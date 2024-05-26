#ifndef KSP_LAUNCH_APP_H
#define KSP_LAUNCH_APP_H

#include <gtk/gtk.h>


void start_launch_app();
void update_launcher_dropdown(GtkComboBox *cb_sel_launcher);
void update_profile_dropdown(GtkComboBox *cb_sel_launcher, GtkComboBox *cb_sel_profile);
struct LV * get_all_launcher();
int * get_launcher_ids();

#endif //KSP_LAUNCH_APP_H
