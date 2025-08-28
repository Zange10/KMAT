#ifndef KSP_GUI_MANAGER_H
#define KSP_GUI_MANAGER_H

#include <gtk/gtk.h>
#include "tools/celestial_systems.h"

void start_gui(const char* gui_filepath);

void create_combobox_dropdown_text_renderer(GObject *combo_box, GtkAlign align);

void append_combobox_entry(GtkComboBox *combo_box, char *new_entry);
void remove_combobox_last_entry(GtkComboBox *combo_box);

int get_path_from_file_chooser(char *filepath, char *extension, GtkFileChooserAction action, char *initial_name);

void update_system_dropdown(GtkComboBox *cb_sel_body);
void update_central_body_dropdown(GtkComboBox *cb_sel_central_body, CelestSystem *system);
void update_body_dropdown(GtkComboBox *cb_sel_body, CelestSystem *system);
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