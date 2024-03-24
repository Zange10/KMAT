#ifndef KSP_PORKCHOP_ANALYZER_H
#define KSP_PORKCHOP_ANALYZER_H

#include <gtk/gtk.h>

void init_porkchop_analyzer(GtkBuilder *builder);
void on_porkchop_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_preview_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_load_itineraries(GtkWidget* widget, gpointer data);
void free_all_porkchop_analyzer_itins();
void on_last_transfer_type_changed_pa(GtkWidget* widget, gpointer data);
void on_apply_filter(GtkWidget* widget, gpointer data);
void on_reset_filter(GtkWidget* widget, gpointer data);

#endif //KSP_PORKCHOP_ANALYZER_H
