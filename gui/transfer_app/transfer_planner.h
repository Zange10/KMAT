#ifndef KSP_TRANSFER_PLANNER_H
#define KSP_TRANSFER_PLANNER_H


#include <gtk/gtk.h>
#include "ephem.h"

void init_transfer_planner(GtkBuilder *builder);
void update();
void update_date_label();
void update_transfer_panel();
void remove_all_transfers();
void on_transfer_planner_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_body_toggle(GtkWidget* widget, gpointer data);
void on_change_date(GtkWidget* widget, gpointer data);
void on_calendar_selection(GtkWidget* widget, gpointer data);
void on_prev_transfer(GtkWidget* widget, gpointer data);
void on_next_transfer(GtkWidget* widget, gpointer data);
void on_transfer_body_change(GtkWidget* widget, gpointer data);
void on_last_transfer_type_changed_tp(GtkWidget* widget, gpointer data);
void on_toggle_transfer_date_lock(GtkWidget* widget, gpointer data);
void on_goto_transfer_date(GtkWidget* widget, gpointer data);
void on_transfer_body_select(GtkWidget* widget, gpointer data);
void on_add_transfer(GtkWidget* widget, gpointer data);
void on_remove_transfer(GtkWidget* widget, gpointer data);
void on_find_closest_transfer(GtkWidget* widget, gpointer data);
void on_find_itinerary(GtkWidget* widget, gpointer data);
void on_save_itinerary(GtkWidget* widget, gpointer data);
void on_load_itinerary(GtkWidget* widget, gpointer data);

void end_transfer_planner();

#endif //KSP_TRANSFER_PLANNER_H
