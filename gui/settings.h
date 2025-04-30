#ifndef KSP_SETTINGS_H
#define KSP_SETTINGS_H

#include <gtk/gtk.h>

void init_global_settings(GtkBuilder *builder);
enum DateType get_settings_datetime_type();


// Handler --------------------------
G_MODULE_EXPORT void on_change_datetime_type();

#endif //KSP_SETTINGS_H
