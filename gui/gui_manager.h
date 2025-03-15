#ifndef KSP_GUI_MANAGER_H
#define KSP_GUI_MANAGER_H

#include <gtk/gtk.h>
#include "celestial_bodies.h"

void start_gui();

enum LastTransferType {TF_FLYBY, TF_CAPTURE, TF_CIRC};

void create_combobox_dropdown_text_renderer(GObject *combo_box);

void update_body_dropdown(GtkComboBox *cb_sel_body, struct System *system);

void update_launcher_dropdown(GtkComboBox *cb_sel_launcher);
void update_profile_dropdown(GtkComboBox *cb_sel_launcher, GtkComboBox *cb_sel_profile);
struct LV * get_all_launcher();
int * get_launcher_ids();

#endif //KSP_GUI_MANAGER_H