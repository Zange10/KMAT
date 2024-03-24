#include "porkchop_analyzer.h"
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "gui/drawing.h"
#include "gui/transfer_app.h"

#include <string.h>
#include <locale.h>



int pa_num_deps, pa_num_itins;
struct ItinStep **pa_departures;
struct ItinStep **pa_arrivals;
double *pa_porkchop;
enum LastTransferType pa_last_transfer_type;
GObject *da_pa_porkchop, *da_pa_preview;
struct ItinStep *curr_transfer_pa;
double current_date_pa;
gboolean body_show_status_pa[9];


void init_porkchop_analyzer(GtkBuilder *builder) {
	pa_num_deps = 0;
	pa_num_itins = 0;
	pa_departures = NULL;
	pa_arrivals = NULL;
	pa_porkchop = NULL;
	curr_transfer_pa = NULL;
	pa_last_transfer_type = TF_FLYBY;
	da_pa_porkchop = gtk_builder_get_object(builder, "da_pa_porkchop");
	da_pa_preview = gtk_builder_get_object(builder, "da_pa_preview");
	for(int i = 0; i < 9; i++) body_show_status_pa[i] = 0;
}

void update_body_show_staus() {
	struct ItinStep *step = get_last(curr_transfer_pa);
	for(int i = 0; i < 9; i++) body_show_status_pa[i] = 0;
	if(step == NULL) return;
	while(step->prev != NULL) {
		if(step->body != NULL) body_show_status_pa[step->body->id-1] = 1;
		step = step->prev;
	}
}

void update_porkchop_drawing_area() {
	gtk_widget_queue_draw(GTK_WIDGET(da_pa_porkchop));
}

void update_preview_drawing_area() {
	gtk_widget_queue_draw(GTK_WIDGET(da_pa_preview));
}

void update_best_itin(int num_itins, int fb0_pow1) {
	double min_dv = pa_porkchop[2+1]+pa_porkchop[3+1]*fb0_pow1+pa_porkchop[4+1];
	int min_dv_ind = 0;
	double dv;

	for(int i = 1; i < num_itins; i++) {
		int ind = 1+i*5;
		dv = pa_porkchop[ind+2]+pa_porkchop[ind+3]*fb0_pow1+pa_porkchop[ind+4];

		if(dv < min_dv) {
			min_dv = dv;
			min_dv_ind = i;
		}
	}

	if(curr_transfer_pa != NULL) free_itinerary(get_first(curr_transfer_pa));
	curr_transfer_pa = create_itin_copy_from_arrival(pa_arrivals[min_dv_ind]);
}

void on_porkchop_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 42.0/255,46.0/255,50.0/255);
	cairo_set_source_rgb(cr, 0.15,0.15, 0.15);
	cairo_fill(cr);

	if(pa_porkchop != NULL) draw_porkchop(cr, area_width, area_height, pa_porkchop, pa_last_transfer_type == TF_FLYBY ? 0 : 1);
}

void on_preview_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;
	struct Vector2D center = {(double) area_width/2, (double) area_height/2};

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

	// Scale
	int highest_id = 0;
	update_body_show_staus();
	for(int i = 0; i < 9; i++) if(body_show_status_pa[i]) highest_id = i + 1;
	double scale = calc_scale(area_width, area_height, highest_id);

	// Sun
	set_cairo_body_color(cr, 0);
	draw_body(cr, center, 0, vec(0,0,0));
	cairo_fill(cr);

	// Planets
	for(int i = 0; i < 9; i++) {
		if(body_show_status_pa[i]) {
			int id = i+1;
			set_cairo_body_color(cr, id);
			struct OSV osv = osv_from_ephem(get_body_ephems()[i], current_date_pa, SUN());
			draw_body(cr, center, scale, osv.r);
			draw_orbit(cr, center, scale, osv.r, osv.v, SUN());
		}
	}

	// Transfers
	if(curr_transfer_pa != NULL) {
		struct ItinStep *temp_transfer = get_first(curr_transfer_pa);
		while(temp_transfer != NULL) {
			if(temp_transfer->body != NULL) {
				int id = temp_transfer->body->id;
				set_cairo_body_color(cr, id);
			} else temp_transfer->v_body.x = 1;	// set to 1 to draw trajectory
			draw_transfer_point(cr, center, scale, temp_transfer->r);
			if(temp_transfer->prev != NULL) draw_trajectory(cr, center, scale, temp_transfer);
			temp_transfer = temp_transfer->next != NULL ? temp_transfer->next[0] : NULL;
		}
	}
}

void analyze_departure_itins() {
	int num_itins = 0, tot_num_itins = 0;
	for(int i = 0; i < pa_num_deps; i++) num_itins += get_number_of_itineraries(pa_departures[i]);
	for(int i = 0; i < pa_num_deps; i++) tot_num_itins += get_total_number_of_stored_steps(pa_departures[i]);

	printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins, tot_num_itins);

	int index = 0;
	pa_arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < pa_num_deps; i++) store_itineraries_in_array(pa_departures[i], pa_arrivals, &index);

	pa_porkchop = (double *) malloc((5 * num_itins + 1) * sizeof(double));
	pa_porkchop[0] = 0;
	int fb0_pow1 = pa_last_transfer_type == TF_FLYBY ? 0 : 1;
	for(int i = 0; i < num_itins; i++) {
		create_porkchop_point(pa_arrivals[i], &pa_porkchop[i * 5 + 1], pa_last_transfer_type == TF_CIRC ? 0 : 1);
		pa_porkchop[0] += 5;
	}
	pa_num_itins = num_itins;
	update_best_itin(num_itins, fb0_pow1);
}

void on_load_itineraries(GtkWidget* widget, gpointer data) {
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	// Create the file chooser dialog
	dialog = gtk_file_chooser_dialog_new("Open File", NULL, action,
										 "_Cancel", GTK_RESPONSE_CANCEL,
										 "_Open", GTK_RESPONSE_ACCEPT,
										 NULL);

	// Set initial folder
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "./Itineraries");

	// Create a filter for files with the extension .itin
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.itins");
	gtk_file_filter_set_name(filter, ".itins");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	// Run the dialog
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filepath;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filepath = gtk_file_chooser_get_filename(chooser);

		free_all_porkchop_analyzer_itins();
		pa_num_deps = get_num_of_deps_of_itinerary_from_bfile(filepath);
		pa_departures = load_itineraries_from_bfile(filepath);
		analyze_departure_itins();
		g_free(filepath);
	}

	// Destroy the dialog
	gtk_widget_destroy(dialog);

	update_porkchop_drawing_area();
	update_preview_drawing_area();
}

void free_all_porkchop_analyzer_itins() {
	if(pa_departures != NULL) {
		for(int i = 0; i < pa_num_deps; i++) free(pa_departures[i]);
		free(pa_departures);
		pa_departures = NULL;
	}
	if(pa_arrivals != NULL) free(pa_arrivals);
	pa_arrivals = NULL;
	if(pa_porkchop != NULL) free(pa_porkchop);
	pa_porkchop = NULL;
	if(curr_transfer_pa != NULL) free_itinerary(get_first(curr_transfer_pa));
	curr_transfer_pa = NULL;
}

void on_last_transfer_type_changed_pa(GtkWidget* widget, gpointer data) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) return;
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "fb") == 0) pa_last_transfer_type = TF_FLYBY;
	else if	(strcmp(name, "capture") == 0) pa_last_transfer_type = TF_CAPTURE;
	else if	(strcmp(name, "circ") == 0) pa_last_transfer_type = TF_CIRC;

	if(pa_porkchop != NULL) free(pa_porkchop);
	pa_porkchop = (double *) malloc((5 * pa_num_itins + 1) * sizeof(double));
	pa_porkchop[0] = 0;
	int fb0_pow1 = pa_last_transfer_type == TF_FLYBY ? 0 : 1;
	for(int i = 0; i < pa_num_itins; i++) {
		create_porkchop_point(pa_arrivals[i], &pa_porkchop[i * 5 + 1], pa_last_transfer_type == TF_CIRC ? 0 : 1);
		pa_porkchop[0] += 5;
	}
	update_best_itin(pa_num_itins, fb0_pow1);
	update_porkchop_drawing_area();
	update_preview_drawing_area();
}
