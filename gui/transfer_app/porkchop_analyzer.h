#ifndef KSP_PORKCHOP_ANALYZER_H
#define KSP_PORKCHOP_ANALYZER_H

#include <gtk/gtk.h>

void init_porkchop_analyzer(GtkBuilder *builder);
void on_porkchop_draw(GtkWidget *widget, cairo_t *cr, gpointer data);

#endif //KSP_PORKCHOP_ANALYZER_H
