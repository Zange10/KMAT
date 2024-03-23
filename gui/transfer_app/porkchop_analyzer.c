#include "porkchop_analyzer.h"
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "gui/drawing.h"
#include "gui/transfer_app.h"

#include <string.h>
#include <locale.h>



int pa_num_deps, pa_num_itins;
struct ItinStep **pa_departures;
struct ItinStep **pa_arrivals;
double *pa_porkchop;
enum LastTransferType pa_last_transfer_type;
GObject *da_pa_porkchop;


void init_porkchop_analyzer(GtkBuilder *builder) {
	pa_num_deps = 0;
	pa_num_itins = 0;
	pa_departures = NULL;
	pa_arrivals = NULL;
	pa_porkchop = NULL;
	pa_last_transfer_type = TF_FLYBY;
	da_pa_porkchop = gtk_builder_get_object(builder, "da_pa_porkchop");
}

void update_porkchop_drawing_area() {
	gtk_widget_queue_draw(GTK_WIDGET(da_pa_porkchop));
}

void on_porkchop_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

	draw_porkchop(cr, area_width, area_height, pa_porkchop, pa_last_transfer_type == TF_FLYBY ? 0 : 1);
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
	for(int i = 0; i < num_itins; i++) {
		create_porkchop_point(pa_arrivals[i], &pa_porkchop[i * 5 + 1], pa_last_transfer_type == TF_CIRC ? 0 : 1);
		pa_porkchop[0] += 5;
	}
	pa_num_itins = num_itins;
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
	for(int i = 0; i < pa_num_itins; i++) {
		create_porkchop_point(pa_arrivals[i], &pa_porkchop[i * 5 + 1], pa_last_transfer_type == TF_CIRC ? 0 : 1);
		pa_porkchop[0] += 5;
	}
	update_porkchop_drawing_area();
}
