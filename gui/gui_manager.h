#ifndef KSP_GUI_MANAGER_H
#define KSP_GUI_MANAGER_H

#include <gtk/gtk.h>
#include "celestial_bodies.h"
#include "tools/datetime.h"

void start_gui(const char* gui_filepath);

enum LastTransferType {TF_FLYBY, TF_CAPTURE, TF_CIRC};

void create_combobox_dropdown_text_renderer(GObject *combo_box);

void update_system_dropdown(GtkComboBox *cb_sel_body);
void update_body_dropdown(GtkComboBox *cb_sel_body, struct System *system);
void change_text_field_date_type(GObject *text_field, enum DateType old_date_type, enum DateType new_date_type);
void change_label_date_type(GObject *label, enum DateType old_date_type, enum DateType new_date_type);
void change_button_date_type(GObject *button, enum DateType old_date_type, enum DateType new_date_type);

char * get_itins_directory();
void change_gui_date_type(enum DateType old_date_type, enum DateType new_date_type);

void update_launcher_dropdown(GtkComboBox *cb_sel_launcher);
void update_profile_dropdown(GtkComboBox *cb_sel_launcher, GtkComboBox *cb_sel_profile);
struct LV * get_all_launcher();
int * get_launcher_ids();

#endif //KSP_GUI_MANAGER_H