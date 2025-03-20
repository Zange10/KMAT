#ifndef KSP_PORKCHOP_ANALYZER_H
#define KSP_PORKCHOP_ANALYZER_H

#include <gtk/gtk.h>
#include "tools/datetime.h"

void init_porkchop_analyzer(GtkBuilder *builder);
void pa_change_date_type(enum DateType old_date_type, enum DateType new_date_type);
void free_all_porkchop_analyzer_itins();


// Handler -----------------------
void on_change_itin_group_visibility(GtkWidget* widget, gpointer data);
void on_porkchop_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_preview_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_load_itineraries(GtkWidget* widget, gpointer data);
void on_save_best_itinerary(GtkWidget* widget, gpointer data);
void on_last_transfer_type_changed_pa(GtkWidget* widget, gpointer data);
void on_apply_filter(GtkWidget* widget, gpointer data);
void on_reset_filter(GtkWidget* widget, gpointer data);
void on_prev_transfer_pa(GtkWidget* widget, gpointer data);
void on_next_transfer_pa(GtkWidget* widget, gpointer data);
void on_switch_steps_groups(GtkWidget* widget, gpointer data);

#endif //KSP_PORKCHOP_ANALYZER_H
