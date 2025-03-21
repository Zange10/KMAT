#include "launch_parameter_analyzer.h"
#include "gui/gui_manager.h"
#include "tools/analytic_geometry.h"
#include "launch_calculator/lp_parameters.h"
#include "gui/drawing.h"

GObject *da_lp_disp1;
GObject *da_lp_disp2;
GObject *da_lp_disp3;
GObject *da_lp_disp4;
GObject *cb_lp_sel_launcher;
GObject *tf_lp_lat;
GObject *tf_lp_incl;

struct ProfileDataPoints {
	int num_points;
	double *payload_mass;
	double *a1;
	double *a2;
	double *b2;
	double *dv;
};


struct ProfileDataPoints pdp;

void free_lpa_data_points();
void init_lpa_data_points(int num_points);

void init_launch_parameter_analyzer(GtkBuilder *builder) {
	da_lp_disp1 = gtk_builder_get_object(builder, "da_lp_disp1");
	da_lp_disp2 = gtk_builder_get_object(builder, "da_lp_disp2");
	da_lp_disp3 = gtk_builder_get_object(builder, "da_lp_disp3");
	da_lp_disp4 = gtk_builder_get_object(builder, "da_lp_disp4");
	cb_lp_sel_launcher = gtk_builder_get_object(builder, "cb_lp_sel_launcher");
	tf_lp_lat = gtk_builder_get_object(builder, "tf_lp_lat");
	tf_lp_incl = gtk_builder_get_object(builder, "tf_lp_incl");

	update_launcher_dropdown(GTK_COMBO_BOX(cb_lp_sel_launcher));

	// Create a cell renderer for dropdowns/ComboBox
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_lp_sel_launcher), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cb_lp_sel_launcher), renderer, "text", 0, NULL);
}

G_MODULE_EXPORT void on_launch_parameter_analyze_disp_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;
	int disp_id = (int) strtol(gtk_widget_get_name(widget), NULL, 10);	// char to int

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0.15,0.15, 0.15);
	cairo_fill(cr);

	if(pdp.num_points == 0) return;

	double *x, *y;
	switch(disp_id) {
		case 1:
			x = pdp.payload_mass;
			y = pdp.a1;
			break;
		case 2:
			x = pdp.payload_mass;
			y = pdp.a2;
			break;
		case 3:
			x = pdp.payload_mass;
			y = pdp.b2;
			break;
		default:
			x = pdp.payload_mass;
			y = pdp.dv;
			break;
	}
	draw_plot(cr, area_width, area_height, x, y, pdp.num_points);
}

G_MODULE_EXPORT void on_lp_analyze() {
	int launcher_id = gtk_combo_box_get_active(GTK_COMBO_BOX(cb_lp_sel_launcher));
	struct LV *all_launcher = get_all_launcher();
	int *launcher_ids = get_launcher_ids();

	if(launcher_id < 0) return;
	char *string;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_lp_lat));
	double launch_latitude = deg2rad(strtod(string, NULL));
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_lp_incl));
	double target_incl = deg2rad(strtod(string, NULL));

	init_lpa_data_points(25);
	calc_payload_curve4(all_launcher[launcher_id], pdp.payload_mass, pdp.a1, pdp.a2, pdp.b2, pdp.dv, pdp.num_points);

	gtk_widget_queue_draw(GTK_WIDGET(da_lp_disp1));
	gtk_widget_queue_draw(GTK_WIDGET(da_lp_disp2));
	gtk_widget_queue_draw(GTK_WIDGET(da_lp_disp3));
	gtk_widget_queue_draw(GTK_WIDGET(da_lp_disp4));
}

void init_lpa_data_points(int num_points) {
	if(pdp.num_points != 0) free_lpa_data_points();
	pdp.num_points = num_points;
	pdp.payload_mass = (double*) malloc(sizeof(double) * num_points);
	pdp.a1 = (double*) malloc(sizeof(double) * num_points);
	pdp.a2 = (double*) malloc(sizeof(double) * num_points);
	pdp.b2 = (double*) malloc(sizeof(double) * num_points);
	pdp.dv = (double*) malloc(sizeof(double) * num_points);
}

void free_lpa_data_points() {
	if(pdp.num_points == 0) return;
	pdp.num_points = 0;
	free(pdp.payload_mass);
	free(pdp.a1);
	free(pdp.a2);
	free(pdp.b2);
	free(pdp.dv);
}

void close_launch_parameter_analyzer() {
	free_lpa_data_points();
}
