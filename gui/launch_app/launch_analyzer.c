#include "launch_analyzer.h"
#include "database/lv_database.h"
#include "launch_calculator/lv_profile.h"
#include "tools/analytic_geometry.h"
#include "launch_calculator/launch_sim.h"
#include "launch_calculator/launch_calculator.h"
#include "gui/drawing.h"


GObject *cb_la_sel_launcher;
GObject *cb_la_sel_profile;
GObject *da_la_disp1;
GObject *da_la_disp2;
GObject *lb_la_res_dur;
GObject *lb_la_res_alt;
GObject *lb_la_res_ap;
GObject *lb_la_res_pe;
GObject *lb_la_res_sma;
GObject *lb_la_res_ecc;
GObject *lb_la_res_incl;
GObject *lb_la_res_orbv;
GObject *lb_la_res_surfv;
GObject *lb_la_res_vertv;
GObject *lb_la_res_spentdv;
GObject *lb_la_res_remdv;
GObject *lb_la_res_remfuel;
GObject *lb_la_res_dwnrng;
GObject *tf_la_sim_incl;
GObject *tf_la_sim_lat;
GObject *tf_la_sim_plmass;

struct LV *all_launcher;
int *launcher_ids;
int num_launcher;
struct LaunchState *launch_state;

void on_launch_analyzer_disp1_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_launch_analyzer_disp2_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_run_launch_simulation(GtkWidget* widget, gpointer data);
void on_change_launcher(GtkWidget* widget, gpointer data);
void update_launcher_dropdown();
void update_profile_dropdown();

struct LaunchDataPoints {
	int num_points;
	double *t;		// time [s]
	double *alt; 	// altitude [km]
	double *orbv; 	// orbital speed [m/s]
	double *surfv;	// surface speed [m/s]
	double *vertv;	// vertical speed [m/s]
	double *mass;	// mass [t]
	double *pitch;	// pitch [deg]
	int *stage;
};

struct LaunchDataPoints ldp;

void init_launch_analyzer(GtkBuilder *builder) {
	num_launcher = get_all_launch_vehicles_from_database(&all_launcher, &launcher_ids);
	
	cb_la_sel_profile = gtk_builder_get_object(builder, "cb_la_sel_profile");
	cb_la_sel_launcher = gtk_builder_get_object(builder, "cb_la_sel_launcher");
	da_la_disp1 = gtk_builder_get_object(builder, "da_la_disp1");
	da_la_disp2 = gtk_builder_get_object(builder, "da_la_disp2");
	lb_la_res_dur = gtk_builder_get_object(builder, "lb_la_res_dur");
	lb_la_res_alt = gtk_builder_get_object(builder, "lb_la_res_alt");
	lb_la_res_ap = gtk_builder_get_object(builder, "lb_la_res_ap");
	lb_la_res_pe = gtk_builder_get_object(builder, "lb_la_res_pe");
	lb_la_res_sma = gtk_builder_get_object(builder, "lb_la_res_sma");
	lb_la_res_ecc = gtk_builder_get_object(builder, "lb_la_res_ecc");
	lb_la_res_incl = gtk_builder_get_object(builder, "lb_la_res_incl");
	lb_la_res_orbv = gtk_builder_get_object(builder, "lb_la_res_orbv");
	lb_la_res_surfv = gtk_builder_get_object(builder, "lb_la_res_surfv");
	lb_la_res_vertv = gtk_builder_get_object(builder, "lb_la_res_vertv");
	lb_la_res_spentdv = gtk_builder_get_object(builder, "lb_la_res_spentdv");
	lb_la_res_remdv = gtk_builder_get_object(builder, "lb_la_res_remdv");
	lb_la_res_remfuel = gtk_builder_get_object(builder, "lb_la_res_remfuel");
	lb_la_res_dwnrng = gtk_builder_get_object(builder, "lb_la_res_dwnrng");
	tf_la_sim_incl = gtk_builder_get_object(builder, "tf_la_sim_incl");
	tf_la_sim_lat = gtk_builder_get_object(builder, "tf_la_sim_lat");
	tf_la_sim_plmass = gtk_builder_get_object(builder, "tf_la_sim_plmass");
	
	
	update_launcher_dropdown();
	
	// Create a cell renderer for dropdowns/ComboBox
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_la_sel_launcher), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_la_sel_launcher), renderer, "text", 0, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_la_sel_profile), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_la_sel_profile), renderer, "text", 0, NULL);
}

void free_launch_data_points() {
	if(ldp.t != NULL) {
		free(ldp.t);
		free(ldp.alt);
		free(ldp.orbv);
		free(ldp.surfv);
		free(ldp.vertv);
		free(ldp.mass);
		free(ldp.pitch);
		free(ldp.stage);
	}
}

void update_launch_data_points(struct LaunchState *state, struct Body *body) {
	if(state == NULL) return;
	free_launch_data_points();

	double step = 1; // [s]
	double next_point_t = 0;

	ldp.num_points = (int) (get_last_state(state)->t / step) + 2;

	ldp.t = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.alt = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.orbv = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.surfv = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.vertv = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.mass = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.pitch = (double*) malloc(sizeof(double) * ldp.num_points);
	ldp.stage = (int*) malloc(sizeof(int) * ldp.num_points);

	state = get_first_state(state);
	int counter = 0;

	while(state != NULL) {
		if(state->t >= next_point_t || state->next == NULL) {
			ldp.t[counter] = state->t;
			ldp.alt[counter] = (vector_mag(state->r) - body->radius) / 1000;
			ldp.orbv[counter] = vector_mag(state->v);
			ldp.surfv[counter] = vector_mag(calc_surface_velocity_from_osv(state->r, state->v, body));
			ldp.vertv[counter] = calc_vertical_speed_from_osv(state->r, state->v);
			ldp.mass[counter] = state->m / 1000;
			ldp.pitch[counter] = rad2deg(state->pitch);
			ldp.stage[counter] = state->stage_id;
			counter++;
			next_point_t += step;
		}
		state = state->next;
	}
}

void on_launch_analyzer_disp1_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0.15,0.15, 0.15);
	cairo_fill(cr);

	if(launch_state != NULL) draw_launch_data(cr, area_width, area_height, ldp.t, ldp.pitch, ldp.num_points);
}

void on_launch_analyzer_disp2_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0.15,0.15, 0.15);
	cairo_fill(cr);

	if(launch_state != NULL) draw_launch_data(cr, area_width, area_height, ldp.t, ldp.orbv, ldp.num_points);
}


void update_launcher_dropdown() {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < num_launcher; i++) {
		gtk_list_store_append(store, &iter);
		char entry[30];
		sprintf(entry, "%s", all_launcher[i].name);
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}
	
	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_la_sel_launcher), GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_la_sel_launcher), 0);
	
	g_object_unref(store);
}

void update_profile_dropdown() {
	int id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_la_sel_launcher));
	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(launcher_ids[id]);
	
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkTreeIter iter;
	// Add items to the list store
	for(int i = 0; i < profiles.num_profiles; i++) {
		gtk_list_store_append(store, &iter);
		char entry[50];
		switch(profiles.profile[i].profiletype) {
			case 1: sprintf(entry, "p = %.0f", profiles.profile[i].lp_params[0]);
				break;
			case 2: sprintf(entry, "p = 90*exp(-%f*h)", profiles.profile[i].lp_params[0]);
				break;
			case 3: sprintf(entry, "p = (90-%.0f)*exp(-%f*h) + %.0f",
							profiles.profile[i].lp_params[1],
							profiles.profile[i].lp_params[0],
							profiles.profile[i].lp_params[1]);
				break;
			case 4: sprintf(entry, "p4(%f, %f, %f)",
						   profiles.profile[i].lp_params[0],
						   profiles.profile[i].lp_params[2],
						   profiles.profile[i].lp_params[1]);
				break;
		}
		gtk_list_store_set(store, &iter, 0, entry, 1, i, -1);
	}
	
	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_la_sel_profile), GTK_TREE_MODEL(store));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_la_sel_profile), 0);
	
	g_object_unref(store);
}

void update_launch_result_values(double dur, double alt, double ap, double pe, double sma, double ecc, double incl,
								 double orbv, double surfv, double vertv, double spentdv, double remdv, double remfuel, double dwnrng) {
	char value_string[20];
	sprintf(value_string, "%.0f s", dur);
	gtk_label_set_label(GTK_LABEL(lb_la_res_dur), value_string);
	sprintf(value_string, "%.0f km", alt/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_alt), value_string);
	sprintf(value_string, "%.0f km", ap/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_ap), value_string);
	sprintf(value_string, "%.0f km", pe/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_pe), value_string);
	sprintf(value_string, "%.0f km", sma/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_sma), value_string);
	sprintf(value_string, "%.3f", ecc);
	gtk_label_set_label(GTK_LABEL(lb_la_res_ecc), value_string);
	sprintf(value_string, "%.2f°", rad2deg(incl));
	gtk_label_set_label(GTK_LABEL(lb_la_res_incl), value_string);

	sprintf(value_string, "%.0f m/s", orbv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_orbv), value_string);
	sprintf(value_string, "%.0f m/s", surfv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_surfv), value_string);
	sprintf(value_string, "%.0f m/s", vertv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_vertv), value_string);
	sprintf(value_string, "%.0f m/s", spentdv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_spentdv), value_string);
	sprintf(value_string, "%.0f m/s", remdv);
	gtk_label_set_label(GTK_LABEL(lb_la_res_remdv), value_string);
	sprintf(value_string, "%.0f kg", remfuel);
	gtk_label_set_label(GTK_LABEL(lb_la_res_remfuel), value_string);
	sprintf(value_string, "%.0f km", dwnrng/1000);
	gtk_label_set_label(GTK_LABEL(lb_la_res_dwnrng), value_string);
}

void on_change_launcher(GtkWidget* widget, gpointer data) {
	update_profile_dropdown();
}


void on_run_launch_simulation(GtkWidget* widget, gpointer data) {
	int launcher_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_la_sel_launcher));
	int profile_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_la_sel_profile));
	if(launcher_id < 0 || profile_id < 0) return;

	struct LaunchProfiles_DB profiles = db_get_launch_profiles_from_lv_id(launcher_ids[launcher_id]);

	char *string;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_la_sim_lat));
	double launch_latitude = deg2rad(strtod(string, NULL));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_la_sim_incl));
	double target_incl = deg2rad(strtod(string, NULL));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_la_sim_plmass));
	double payload_mass = strtod(string, NULL) * 1000;
	
	all_launcher[launcher_id].lp_id = profiles.profile[profile_id].profiletype;
	for(int i = 0; i < 5; i++) all_launcher[launcher_id].lp_params[i] = profiles.profile[profile_id].lp_params[i];
	
	print_LV(&all_launcher[launcher_id]);

	struct Launch_Results lr = run_launch_simulation(all_launcher[launcher_id], payload_mass, launch_latitude, target_incl, 0.001, 1, 1);

	if(launch_state != NULL) free_launch_states(launch_state);
	launch_state = get_last_state(lr.state);

	struct Body *body = EARTH();

	struct Orbit orbit = constr_orbit_from_osv(launch_state->r, launch_state->v, body);

	update_launch_result_values(
			launch_state->t,
			vector_mag(launch_state->r) - body->radius,
			calc_orbit_apoapsis(orbit),
			calc_orbit_periapsis(orbit),
			orbit.a,
			orbit.e,
			orbit.inclination,
			vector_mag(launch_state->v),
			vector_mag(calc_surface_velocity_from_osv(launch_state->r, launch_state->v, body)),
			calc_vertical_speed_from_osv(launch_state->r, launch_state->v),
			lr.dv,
			lr.rem_dv,
			lr.rf,
			calc_downrange_distance(launch_state->r, launch_state->t, launch_latitude, body)
			);

	update_launch_data_points(launch_state, body);

	for(int i = 0; i < ldp.num_points; i++) {
		printf("%f s   %f km   %f m/s   %f m/s     %f m/s     %f t      %f°    %d\n", ldp.t[i], ldp.alt[i], ldp.surfv[i], ldp.orbv[i], ldp.vertv[i], ldp.mass[i],
			   ldp.pitch[i], ldp.stage[i]);
	}
}

void close_launch_analyzer() {
	if(launch_state != NULL) free_launch_states(launch_state);
	launch_state = NULL;
	free_launch_data_points();
}
