#ifndef KSP_LAUNCH_PARAMETER_ANALYZER_H
#define KSP_LAUNCH_PARAMETER_ANALYZER_H

#include <gtk/gtk.h>

void init_launch_parameter_analyzer(GtkBuilder *builder);
void close_launch_parameter_analyzer();



// Handler --------------------------------------
G_MODULE_EXPORT void on_launch_parameter_analyze_disp_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
G_MODULE_EXPORT void on_lp_analyze();

#endif //KSP_LAUNCH_PARAMETER_ANALYZER_H
