#include "capability_analyzer.h"
#include "launch_calculator/lv_profile.h"
#include "gui/launch_app.h"
#include "database/lv_database.h"
#include "tools/analytic_geometry.h"
#include "launch_calculator/capability_calculator.h"
#include "gui/drawing.h"


GObject *da_ca_disp1;
GObject *da_ca_disp2;
GObject *da_ca_disp3;
GObject *da_ca_disp4;
GObject *cb_ca_sel_launcher;
GObject *cb_ca_sel_profile;
GObject *tf_ca_lat;
GObject *tf_ca_incl;


struct CapabilityDataPoints {
	int num_points;
	double *incl;
	double *incl_plmass;
	double *pl_mass;
	double *apoapsis;
	double *periapsis;
	double *rem_dv;
	double *poss_apo;
};


struct CapabilityDataPoints cdp;


void free_capability_data_points();
void init_capability_data_points(int num_points);

void init_capability_analyzer(GtkBuilder *builder) {
	da_ca_disp1 = gtk_builder_get_object(builder, "da_ca_disp1");
	da_ca_disp2 = gtk_builder_get_object(builder, "da_ca_disp2");
	da_ca_disp3 = gtk_builder_get_object(builder, "da_ca_disp3");
	da_ca_disp4 = gtk_builder_get_object(builder, "da_ca_disp4");
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

	cdp.num_points = 0;
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

	if(cdp.num_points == 0) return;

	if(disp_id != 2) {
		double *x, *y;
		switch(disp_id) {
			case 1:
				x = cdp.incl;
				y = cdp.incl_plmass;
				break;
			case 3:
				x = cdp.pl_mass;
				y = cdp.rem_dv;
				break;
			case 4:
				x = cdp.pl_mass;
				y = cdp.poss_apo;
				break;
			default:
				return;
		}
		draw_plot(cr, area_width, area_height, x, y, cdp.num_points);
	} else {
		double *x = cdp.pl_mass;
		double *y[] = {cdp.apoapsis, cdp.periapsis};
		draw_multi_plot(cr, area_width, area_height, x, y, 2, cdp.num_points);
	}
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

	init_capability_data_points(25);

	calc_payload_per_inclination(all_launcher[launcher_id], launch_latitude, cdp.incl, cdp.incl_plmass, cdp.num_points);
	calc_capability_for_inclination(all_launcher[launcher_id], launch_latitude, target_incl, cdp.pl_mass, cdp.apoapsis, cdp.periapsis, cdp.rem_dv, cdp.poss_apo, cdp.num_points);

	for(int i = 0; i < cdp.num_points; i++) {
		cdp.incl[i] = rad2deg(cdp.incl[i]);
		cdp.incl_plmass[i] /= 1000;
		cdp.pl_mass[i] /= 1000;
		cdp.apoapsis[i] /= 1000;
		cdp.periapsis[i] /= 1000;
		cdp.poss_apo[i] /= 1000;
	}
	gtk_widget_queue_draw(GTK_WIDGET(da_ca_disp1));
	gtk_widget_queue_draw(GTK_WIDGET(da_ca_disp2));
	gtk_widget_queue_draw(GTK_WIDGET(da_ca_disp3));
	gtk_widget_queue_draw(GTK_WIDGET(da_ca_disp4));
}

void init_capability_data_points(int num_points) {
	if(cdp.num_points != 0) free_capability_data_points();
	cdp.num_points = num_points;
	cdp.incl = (double*) malloc(sizeof(double) * num_points);
	cdp.apoapsis = (double*) malloc(sizeof(double) * num_points);
	cdp.rem_dv = (double*) malloc(sizeof(double) * num_points);
	cdp.incl_plmass = (double*) malloc(sizeof(double) * num_points);
	cdp.periapsis = (double*) malloc(sizeof(double) * num_points);
	cdp.pl_mass = (double*) malloc(sizeof(double) * num_points);
	cdp.poss_apo = (double*) malloc(sizeof(double) * num_points);
}

void free_capability_data_points() {
	if(cdp.num_points == 0) return;
	cdp.num_points = 0;
	free(cdp.incl);
	free(cdp.apoapsis);
	free(cdp.rem_dv);
	free(cdp.incl_plmass);
	free(cdp.periapsis);
	free(cdp.pl_mass);
	free(cdp.poss_apo);
}

void close_capability_analyzer() {
	free_capability_data_points();
}
