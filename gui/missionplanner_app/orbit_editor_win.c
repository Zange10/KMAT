#include "orbit_editor_win.h"
#include <math.h>

GObject *orb_editor_win;
GObject *sl_ow_sma;
GObject *sl_ow_ecc;
GObject *sl_ow_incl;
GObject *sl_ow_raan;
GObject *sl_ow_argp;
GObject *sl_ow_ta;
GObject *tf_ow_sma;
GObject *tf_ow_ecc;
GObject *tf_ow_incl;
GObject *tf_ow_raan;
GObject *tf_ow_argp;
GObject *tf_ow_ta;

Orbit *ow_orbit;
bool updating_values = true;

void (*ow_ext_update_func)();

void update_ow_orbit_values();

void set_ow_slider_params(GtkRange *range_obj, double min, double max, double incr) {
	GtkAdjustment *adj = gtk_range_get_adjustment(range_obj);
	gtk_adjustment_set_lower(adj, min);
	gtk_adjustment_set_upper(adj, max);
	gtk_adjustment_set_step_increment(adj, incr);
}

void init_orbit_editor_window(GtkBuilder *builder) {
	orb_editor_win = gtk_builder_get_object(builder, "orb_editor_win");
	sl_ow_sma      = gtk_builder_get_object(builder, "sl_ow_sma");
	sl_ow_ecc      = gtk_builder_get_object(builder, "sl_ow_ecc");
	sl_ow_incl     = gtk_builder_get_object(builder, "sl_ow_incl");
	sl_ow_raan     = gtk_builder_get_object(builder, "sl_ow_raan");
	sl_ow_argp     = gtk_builder_get_object(builder, "sl_ow_argp");
	sl_ow_ta       = gtk_builder_get_object(builder, "sl_ow_ta");

	tf_ow_sma      = gtk_builder_get_object(builder, "tf_ow_sma");
	tf_ow_ecc      = gtk_builder_get_object(builder, "tf_ow_ecc");
	tf_ow_incl     = gtk_builder_get_object(builder, "tf_ow_incl");
	tf_ow_raan     = gtk_builder_get_object(builder, "tf_ow_raan");
	tf_ow_argp     = gtk_builder_get_object(builder, "tf_ow_argp");
	tf_ow_ta       = gtk_builder_get_object(builder, "tf_ow_ta");
	
	
	set_ow_slider_params(GTK_RANGE(sl_ow_sma ), 0,   5, 0.01);
	set_ow_slider_params(GTK_RANGE(sl_ow_ecc ), 0,   5, 0.01);
	set_ow_slider_params(GTK_RANGE(sl_ow_incl), 0, 180, 0.10);
	set_ow_slider_params(GTK_RANGE(sl_ow_raan), 0, 360, 0.10);
	set_ow_slider_params(GTK_RANGE(sl_ow_argp), 0, 360, 0.10);
	set_ow_slider_params(GTK_RANGE(sl_ow_ta  ), 0, 360, 0.10);
}

void show_orbit_editor_window(Orbit *orbit, void (*ext_update_func)()) {
	ow_orbit = orbit;
	ow_ext_update_func = ext_update_func;
	gtk_widget_set_visible(GTK_WIDGET(orb_editor_win), 1);
	update_ow_orbit_values();
}

void update_ow_orbit_values() {
	updating_values = true;
	
	if(ow_orbit->a > 0 != ow_orbit->e < 1) ow_orbit->a *= -1;
	
	gtk_range_set_value(GTK_RANGE(sl_ow_sma), log10(fabs(ow_orbit->a) / ow_orbit->cb->radius));
	gtk_range_set_value(GTK_RANGE(sl_ow_ecc), ow_orbit->e);
	gtk_range_set_value(GTK_RANGE(sl_ow_incl), rad2deg(ow_orbit->i));
	gtk_range_set_value(GTK_RANGE(sl_ow_raan), rad2deg(ow_orbit->raan));
	gtk_range_set_value(GTK_RANGE(sl_ow_argp), rad2deg(ow_orbit->arg_peri));
	gtk_range_set_value(GTK_RANGE(sl_ow_ta), rad2deg(ow_orbit->ta));
	
	char value[32];
	sprintf(value, "%.0f km", ow_orbit->a/1e3);
	gtk_entry_set_text(GTK_ENTRY(tf_ow_sma), value);
	sprintf(value, "%.4f", ow_orbit->e);
	gtk_entry_set_text(GTK_ENTRY(tf_ow_ecc), value);
	sprintf(value, "%.2f °", rad2deg(ow_orbit->i));
	gtk_entry_set_text(GTK_ENTRY(tf_ow_incl), value);
	sprintf(value, "%.2f °", rad2deg(ow_orbit->raan));
	gtk_entry_set_text(GTK_ENTRY(tf_ow_raan), value);
	sprintf(value, "%.2f °", rad2deg(ow_orbit->arg_peri));
	gtk_entry_set_text(GTK_ENTRY(tf_ow_argp), value);
	sprintf(value, "%.2f °", rad2deg(ow_orbit->ta));
	gtk_entry_set_text(GTK_ENTRY(tf_ow_ta), value);
	updating_values = false;
}

G_MODULE_EXPORT void on_ow_slider_change(GtkWidget* widget) {
	if(updating_values) return;
	if(widget == sl_ow_sma) ow_orbit->a = pow(10,gtk_range_get_value(GTK_RANGE(sl_ow_sma)))*ow_orbit->cb->radius;
	if(widget == sl_ow_ecc) ow_orbit->e = gtk_range_get_value(GTK_RANGE(sl_ow_ecc));
	if(widget == sl_ow_incl) ow_orbit->i = deg2rad(gtk_range_get_value(GTK_RANGE(sl_ow_incl)));
	if(widget == sl_ow_raan) ow_orbit->raan = deg2rad(gtk_range_get_value(GTK_RANGE(sl_ow_raan)));
	if(widget == sl_ow_argp) ow_orbit->arg_peri = deg2rad(gtk_range_get_value(GTK_RANGE(sl_ow_argp)));
	if(widget == sl_ow_ta) ow_orbit->ta = deg2rad(gtk_range_get_value(GTK_RANGE(sl_ow_ta)));
	update_ow_orbit_values();
	ow_ext_update_func();
}

G_MODULE_EXPORT void on_ow_entry_value_change(GtkWidget* widget) {
	if(updating_values) return;
	char *string;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(widget));
	if(widget == tf_ow_sma) ow_orbit->a = strtod(string, NULL)*1e3;
	if(widget == tf_ow_ecc) ow_orbit->e = strtod(string, NULL);
	if(widget == tf_ow_incl) ow_orbit->i = deg2rad(strtod(string, NULL));
	if(widget == tf_ow_raan) ow_orbit->raan = deg2rad(strtod(string, NULL));
	if(widget == tf_ow_argp) ow_orbit->arg_peri = deg2rad(strtod(string, NULL));
	if(widget == tf_ow_ta) ow_orbit->ta = deg2rad(strtod(string, NULL));
	update_ow_orbit_values();
	ow_ext_update_func();
}


G_MODULE_EXPORT gboolean on_hide_orbit_editor_window(GtkWidget* widget) {
	gtk_widget_set_visible(GTK_WIDGET(orb_editor_win), 0);
	return TRUE;
}