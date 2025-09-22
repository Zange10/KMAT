#ifndef KMAT_ORBIT_EDITOR_WIN_H
#define KMAT_ORBIT_EDITOR_WIN_H

#include <gtk/gtk.h>
#include "orbitlib.h"

void init_orbit_editor_window(GtkBuilder *builder);
void show_orbit_editor_window(Orbit *orbit, void (*ext_update_func)());


G_MODULE_EXPORT void on_ow_slider_change(GtkWidget* widget);
G_MODULE_EXPORT void on_ow_entry_value_change(GtkWidget* widget);
G_MODULE_EXPORT gboolean on_hide_orbit_editor_window(GtkWidget* widget);

#endif //KMAT_ORBIT_EDITOR_WIN_H
