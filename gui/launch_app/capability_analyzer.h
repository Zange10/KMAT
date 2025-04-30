#ifndef KSP_CAPABILITY_CALCULATOR_H
#define KSP_CAPABILITY_CALCULATOR_H

#include <gtk/gtk.h>

void init_capability_analyzer(GtkBuilder *builder);
void close_capability_analyzer();


// Handler ------------------------------------
G_MODULE_EXPORT void on_ca_change_launcher(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_capability_analyzer_disp_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
G_MODULE_EXPORT void on_ca_analyze();

#endif //KSP_CAPABILITY_CALCULATOR_H
