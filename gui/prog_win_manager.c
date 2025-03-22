#include "prog_win_manager.h"
#include "orbit_calculator/transfer_calc.h"
#include "tools/thread_pool.h"


GObject *tf_ic_prog_window;
GObject *tf_ic_prog_bar;
GObject *lb_ic_prog_info;


void init_prog_window(GtkBuilder *builder) {
	tf_ic_prog_window = gtk_builder_get_object(builder, "progress_window");
	tf_ic_prog_bar = gtk_builder_get_object(builder, "pb_pw_prog_bar");
	lb_ic_prog_info = gtk_builder_get_object(builder, "lb_pw_prog_info");
}



// Function to update the progress window
static gboolean update_tc_ic_progress_window(gpointer data) {
	if (!gtk_widget_is_visible(GTK_WIDGET(tf_ic_prog_window))) {
		return G_SOURCE_REMOVE;  // Stop the timeout
	}
	struct Transfer_Calc_Status calc_status = get_current_transfer_calc_status();
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tf_ic_prog_bar), calc_status.progress);
	char s[64];
	if(get_thread_counter(0) <= calc_status.jd_diff)
		sprintf(s, "Departure dates analyzed: %d / %d", calc_status.num_deps, (int) calc_status.jd_diff);
	else
		sprintf(s, "Departures dates analyzed: %d / %d\nEnding Calculations...", calc_status.num_deps, (int) calc_status.jd_diff);
	gtk_label_set_text(GTK_LABEL(lb_ic_prog_info), s);

	return G_SOURCE_CONTINUE;  // Continue updating
}

void init_tc_ic_progress_window() {
	gtk_widget_set_visible(GTK_WIDGET(tf_ic_prog_window), 1);
	// update progress window every 0.1s
	g_timeout_add(100, update_tc_ic_progress_window, NULL);
}

void end_tc_ic_progress_window() {
	gtk_widget_set_visible(GTK_WIDGET(tf_ic_prog_window), 0);
}

G_MODULE_EXPORT void end_progress_calculation() {
	struct Transfer_Calc_Status calc_status = get_current_transfer_calc_status();
	for(int i = 0; i <= calc_status.jd_diff; i++) {
		get_incr_thread_counter(0);
	}
}
