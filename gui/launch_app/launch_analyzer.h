#ifndef KSP_LAUNCH_ANALYZER_H
#define KSP_LAUNCH_ANALYZER_H

#include <gtk/gtk.h>

void init_launch_analyzer(GtkBuilder *builder);
void close_launch_analyzer();


// Handler ----------------------------------------
G_MODULE_EXPORT void on_launch_analyzer_disp_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
G_MODULE_EXPORT void on_launch_analyzer_disp2_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
G_MODULE_EXPORT void on_la_disp_sel(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_change_la_display_xvariable(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_change_la_display_yvariable(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_run_launch_simulation(GtkWidget* widget, gpointer data);
G_MODULE_EXPORT void on_change_launcher(GtkWidget* widget, gpointer data);

#endif //KSP_LAUNCH_ANALYZER_H
