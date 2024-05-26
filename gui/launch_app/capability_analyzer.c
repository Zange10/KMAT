#include "capability_analyzer.h"
#include "launch_calculator/lv_profile.h"
#include "gui/launch_app.h"
#include "database/lv_database.h"
#include "tools/analytic_geometry.h"


GObject *cb_ca_sel_launcher;
GObject *cb_ca_sel_profile;
GObject *tf_ca_lat;
GObject *tf_ca_incl;


void init_capability_analyzer(GtkBuilder *builder) {
	cb_ca_sel_launcher = gtk_builder_get_object(builder, "cb_ca_sel_launcher");
	cb_ca_sel_profile = gtk_builder_get_object(builder, "cb_ca_sel_profile");
	tf_ca_lat = gtk_builder_get_object(builder, "tf_ca_lat");
	tf_ca_incl = gtk_builder_get_object(builder, "tf_ca_incl");

	update_launcher_dropdown(GTK_COMBO_BOX(cb_ca_sel_launcher));

	// Create a cell renderer for dropdowns/ComboBox
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_ca_sel_launcher), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_ca_sel_launcher), renderer, "text", 0, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_ca_sel_profile), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_ca_sel_profile), renderer, "text", 0, NULL);
}

void on_ca_change_launcher(GtkWidget* widget, gpointer data) {
	update_profile_dropdown(GTK_COMBO_BOX(cb_ca_sel_launcher), GTK_COMBO_BOX(cb_ca_sel_profile));
}

void on_capability_analyzer_disp_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;
	int disp_id = (int) strtol(gtk_widget_get_name(widget), NULL, 10);	// char to int

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0.15,0.15, 0.15);
	cairo_fill(cr);

	double *x, *y;
	switch(disp_id) {
		case 1: break;
		case 2: break;
		case 3: break;
		case 4: break;
		default: return;
	}
//	if(launch_state != NULL) draw_plot(cr, area_width, area_height, x, y, ldp.num_points);
}

void on_ca_analyze() {
	int launcher_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ca_sel_launcher));
	int profile_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_ca_sel_profile));
	struct LV *all_launcher = get_all_launcher();
	int *launcher_ids = get_launcher_ids();

	if(launcher_id < 0 || profile_id < 0) return;

	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(launcher_ids[launcher_id]);

	char *string;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ca_lat));
	double launch_latitude = deg2rad(strtod(string, NULL));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_ca_incl));
	double target_incl = deg2rad(strtod(string, NULL));

	all_launcher[launcher_id].lp_id = profiles.profile[profile_id].profiletype;
	for(int i = 0; i < 5; i++) all_launcher[launcher_id].lp_params[i] = profiles.profile[profile_id].lp_params[i];

	print_LV(&all_launcher[launcher_id]);

//	struct Launch_Results lr = run_launch_simulation(all_launcher[launcher_id], payload_mass, launch_latitude, target_incl, 0.001, 1, 1, coast_time_after_launch);
//
//	if(launch_state != NULL) free_launch_states(launch_state);
//	launch_state = get_last_state(lr.state);
//
//	struct Body *body = EARTH();
//
//	struct Orbit orbit = constr_orbit_from_osv(launch_state->r, launch_state->v, body);
//
//	update_launch_result_values(
//			launch_state->t,
//			vector_mag(launch_state->r) - body->radius,
//			calc_orbit_apoapsis(orbit),
//			calc_orbit_periapsis(orbit),
//			orbit.a,
//			orbit.e,
//			orbit.inclination,
//			vector_mag(launch_state->v),
//			vector_mag(calc_surface_velocity_from_osv(launch_state->r, launch_state->v, body)),
//			calc_vertical_speed_from_osv(launch_state->r, launch_state->v),
//			lr.dv,
//			lr.rem_dv,
//			lr.rf,
//			calc_downrange_distance(launch_state->r, launch_state->t, launch_latitude, body)
//	);
//
//	update_launch_data_points(launch_state, body);
}

void close_capability_analyzer() {

}
