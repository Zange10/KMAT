#ifndef KMAT_MISSION_EDITOR_H
#define KMAT_MISSION_EDITOR_H


#include <gtk/gtk.h>

void init_mission_editor(GtkBuilder *builder);



G_MODULE_EXPORT void on_load_mission_from_itinerary(GtkWidget* widget, gpointer data);
#endif //KMAT_MISSION_EDITOR_H
