#include "info_win_manager.h"
#include "orbit_calculator/transfer_calc.h"
#include "tools/thread_pool.h"


GObject *prog_window;
GObject *tf_prog_bar;
GObject *lb_prog_info;

GObject *msg_window;
GObject *lb_mw_msg;


void init_info_windows(GtkBuilder *builder) {
	prog_window = gtk_builder_get_object(builder, "progress_window");
	tf_prog_bar = gtk_builder_get_object(builder, "pb_pw_prog_bar");
	lb_prog_info = gtk_builder_get_object(builder, "lb_pw_prog_info");

	msg_window = gtk_builder_get_object(builder, "msg_window");
	lb_mw_msg = gtk_builder_get_object(builder, "lb_mw_msg");
}



// Function to update the progress window
static gboolean update_sc_ic_progress_window(gpointer data) {
	if (!gtk_widget_is_visible(GTK_WIDGET(prog_window))) {
		return G_SOURCE_REMOVE;  // Stop the timeout
	}
	struct Transfer_Calc_Status calc_status = get_current_transfer_calc_status();
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tf_prog_bar), calc_status.progress);
	char s[200];
	if(get_thread_counter(3) == 0)
		sprintf(s, "Departure dates analyzed: %d / %d\nItineraries found: %d", calc_status.num_deps, (int) calc_status.jd_diff, calc_status.num_itins);
	else
		sprintf(s, "Departures dates analyzed: %d / %d\nItineraries found: %d\nEnding Calculations...", calc_status.num_deps, (int) calc_status.jd_diff, calc_status.num_itins);
	gtk_label_set_text(GTK_LABEL(lb_prog_info), s);

	return G_SOURCE_CONTINUE;  // Continue updating
}

void init_sc_ic_progress_window() {
	gtk_widget_set_visible(GTK_WIDGET(prog_window), 1);
	// update progress window every 0.1s
	g_timeout_add(100, update_sc_ic_progress_window, NULL);
}

void end_sc_ic_progress_window() {
	gtk_widget_set_visible(GTK_WIDGET(prog_window), 0);
}

G_MODULE_EXPORT void end_progress_calculation() {
	get_incr_thread_counter(3);
}

void show_msg_window(char *msg) {
	gtk_label_set_text(GTK_LABEL(lb_mw_msg), msg);
	gtk_widget_set_visible(GTK_WIDGET(msg_window), 1);
}

G_MODULE_EXPORT gboolean on_hide_msg_window() {
	gtk_widget_set_visible(GTK_WIDGET(msg_window), 0);
	return TRUE;
}

G_MODULE_EXPORT void on_msg_window_ok() {
	gtk_widget_set_visible(GTK_WIDGET(msg_window), 0);
}

