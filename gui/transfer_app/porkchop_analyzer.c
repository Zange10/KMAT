#include "porkchop_analyzer.h"
#include "porkchop_analyzer_tools.h"
#include "celestial_bodies.h"
#include "gui/drawing.h"
#include "gui/transfer_app.h"
#include "gui/css_loader.h"

#include <string.h>
#include <locale.h>
#include <math.h>


int pa_num_deps, pa_num_itins, pa_all_num_itins;
struct ItinStep **pa_departures;
struct ItinStep **pa_arrivals, **pa_all_arrivals;
double *pa_porkchop, *pa_all_porkchop;
enum LastTransferType pa_last_transfer_type;
GObject *da_pa_porkchop, *da_pa_preview;
struct ItinStep *curr_transfer_pa;
double current_date_pa;
int pa_num_groups = 0;
struct Group *pa_groups;

gboolean body_show_status_pa[9];
GObject *tf_pa_min_feedback[5];
GObject *tf_pa_max_feedback[5];
GObject *lb_pa_tfdate;
GObject *lb_pa_tfbody;
GObject *lb_pa_transfer_dv;
GObject *lb_pa_total_dv;
GObject *lb_pa_periapsis;
GObject *st_pa_step_group_selector;
GObject *vp_pa_groups;
GtkWidget *grid_pa_groups;



void init_porkchop_analyzer(GtkBuilder *builder) {
	pa_num_deps = 0;
	pa_num_itins = 0;
	pa_departures = NULL;
	pa_arrivals = NULL;
	pa_porkchop = NULL;
	pa_all_arrivals = NULL;
	pa_all_porkchop = NULL;
	curr_transfer_pa = NULL;
	pa_last_transfer_type = TF_FLYBY;
	da_pa_porkchop = gtk_builder_get_object(builder, "da_pa_porkchop");
	da_pa_preview = gtk_builder_get_object(builder, "da_pa_preview");
	tf_pa_min_feedback[0] = gtk_builder_get_object(builder, "tf_pa_min_depdate");
	tf_pa_min_feedback[1] = gtk_builder_get_object(builder, "tf_pa_min_dur");
	tf_pa_min_feedback[2] = gtk_builder_get_object(builder, "tf_pa_min_totdv");
	tf_pa_min_feedback[3] = gtk_builder_get_object(builder, "tf_pa_min_depdv");
	tf_pa_min_feedback[4] = gtk_builder_get_object(builder, "tf_pa_min_satdv");
	tf_pa_max_feedback[0] = gtk_builder_get_object(builder, "tf_pa_max_depdate");
	tf_pa_max_feedback[1] = gtk_builder_get_object(builder, "tf_pa_max_dur");
	tf_pa_max_feedback[2] = gtk_builder_get_object(builder, "tf_pa_max_totdv");
	tf_pa_max_feedback[3] = gtk_builder_get_object(builder, "tf_pa_max_depdv");
	tf_pa_max_feedback[4] = gtk_builder_get_object(builder, "tf_pa_max_satdv");
	lb_pa_tfdate = gtk_builder_get_object(builder, "lb_pa_tfdate");
	lb_pa_tfbody = gtk_builder_get_object(builder, "lb_pa_tfbody");
	lb_pa_transfer_dv = gtk_builder_get_object(builder, "lb_pa_transfer_dv");
	lb_pa_total_dv = gtk_builder_get_object(builder, "lb_pa_total_dv");
	lb_pa_periapsis = gtk_builder_get_object(builder, "lb_pa_periapsis");
	st_pa_step_group_selector = gtk_builder_get_object(builder, "st_pa_step_group_selector");
	vp_pa_groups = gtk_builder_get_object(builder, "vp_pa_groups");
	for(int i = 0; i < 9; i++) body_show_status_pa[i] = 0;
}

double calc_step_dv_pa(struct ItinStep *step) {
	if(step == NULL || (step->prev == NULL && step->next == NULL)) return 0;
	if(step->body == NULL) {
		if(step->next == NULL || step->next[0]->next == NULL || step->prev == NULL)
			return 0;
		return vector_mag(subtract_vectors(step->v_arr, step->next[0]->v_dep));
	} else if(step->prev == NULL) {
		double vinf = vector_mag(subtract_vectors(step->next[0]->v_dep, step->v_body));
		return dv_circ(step->body, step->body->atmo_alt+100e3, vinf);
	} else if(step->next == NULL) {
		if(pa_last_transfer_type == TF_FLYBY) return 0;
		double vinf = vector_mag(subtract_vectors(step->v_arr, step->v_body));
		if(pa_last_transfer_type == TF_CAPTURE) return dv_capture(step->body, step->body->atmo_alt + 100e3, vinf);
		else if(pa_last_transfer_type == TF_CIRC) return dv_circ(step->body, step->body->atmo_alt + 100e3, vinf);
	}
	return 0;
}

double calc_total_dv_pa() {
	if(curr_transfer_pa == NULL) return 0;
	double dv = 0;
	struct ItinStep *step = get_last(curr_transfer_pa);
	while(step != NULL) {
		dv += calc_step_dv_pa(step);
		step = step->prev;
	}
	return dv;
}

double calc_periapsis_height_pa() {
	if(curr_transfer_pa->body == NULL) return -1e20;
	if(curr_transfer_pa->next == NULL || curr_transfer_pa->prev == NULL) return -1e20;
	struct Vector v_arr = subtract_vectors(curr_transfer_pa->v_arr, curr_transfer_pa->v_body);
	struct Vector v_dep = subtract_vectors(curr_transfer_pa->next[0]->v_dep, curr_transfer_pa->v_body);
	double beta = (M_PI - angle_vec_vec(v_arr, v_dep))/2;
	double rp = (1 / cos(beta) - 1) * (curr_transfer_pa->body->mu / (pow(vector_mag(v_arr), 2)));
	return (rp-curr_transfer_pa->body->radius)*1e-3;
}

void free_all_porkchop_analyzer_itins() {
	if(pa_departures != NULL) {
		for(int i = 0; i < pa_num_deps; i++) free(pa_departures[i]);
		free(pa_departures);
		pa_departures = NULL;
	}
	if(pa_all_arrivals != NULL) free(pa_all_arrivals);
	pa_all_arrivals = NULL;
	pa_arrivals = NULL;
	if(pa_all_porkchop != NULL) free(pa_all_porkchop);
	pa_all_porkchop = NULL;
	pa_porkchop = NULL;
	if(curr_transfer_pa != NULL) free_itinerary(get_first(curr_transfer_pa));
	curr_transfer_pa = NULL;
	free(pa_groups);
	pa_groups = NULL;
}

void update_body_show_staus() {
	struct ItinStep *step = get_last(curr_transfer_pa);
	for(int i = 0; i < 9; i++) body_show_status_pa[i] = 0;
	if(step == NULL) return;
	while(step != NULL) {
		if(step->body != NULL) body_show_status_pa[step->body->id-1] = 1;
		step = step->prev;
	}
}

void update_porkchop_drawing_area() {
	gtk_widget_queue_draw(GTK_WIDGET(da_pa_porkchop));
}

void update_preview() {
	gtk_widget_queue_draw(GTK_WIDGET(da_pa_preview));

	if(curr_transfer_pa == NULL) {
		gtk_label_set_label(GTK_LABEL(lb_pa_tfdate), "0000-00-00");
		gtk_label_set_label(GTK_LABEL(lb_pa_tfbody), "Planet");
	} else {
		struct Date date = convert_JD_date(curr_transfer_pa->date);
		char date_string[10];
		date_to_string(date, date_string, 0);
		gtk_label_set_label(GTK_LABEL(lb_pa_tfdate), date_string);
		if(curr_transfer_pa->body != NULL) gtk_label_set_label(GTK_LABEL(lb_pa_tfbody), curr_transfer_pa->body->name);
		else gtk_label_set_label(GTK_LABEL(lb_pa_tfbody), "Deep-Space Man");
		char s_dv[20];
		sprintf(s_dv, "%6.0f m/s", calc_step_dv_pa(curr_transfer_pa));
		gtk_label_set_label(GTK_LABEL(lb_pa_transfer_dv), s_dv);
		sprintf(s_dv, "%6.0f m/s", calc_total_dv_pa());
		gtk_label_set_label(GTK_LABEL(lb_pa_total_dv), s_dv);
		double h = calc_periapsis_height_pa();
		if(curr_transfer_pa->body != NULL && h > -curr_transfer_pa->body->radius*1e-3)
			sprintf(s_dv, "%.0f km", h);
		else sprintf(s_dv, "- km");
		gtk_label_set_label(GTK_LABEL(lb_pa_periapsis), s_dv);
	}
}

void reset_min_max_feedback(int fb0_pow1, int num_itins) {
	double min[5] = {
			/* depdate	*/ pa_porkchop[1+0],
			/* duration	*/ pa_porkchop[1+1],
			/* total dv	*/ pa_porkchop[1+2]+pa_porkchop[1+3]+pa_porkchop[1+4]*fb0_pow1,
			/* dep dv	*/ pa_porkchop[1+2],
			/* sat dv	*/ pa_porkchop[1+3]+pa_porkchop[1+4]*fb0_pow1,
	};
	double max[5] = {
			/* depdate	*/ pa_porkchop[1+0],
			/* duration	*/ pa_porkchop[1+1],
			/* total dv	*/ pa_porkchop[1+2]+pa_porkchop[1+3]+pa_porkchop[1+4]*fb0_pow1,
			/* dep dv	*/ pa_porkchop[1+2],
			/* sat dv	*/ pa_porkchop[1+3]+pa_porkchop[1+4]*fb0_pow1,
	};
	double dep_dv, sat_dv, tot_dv, date, dur;

	for(int i = 1; i < num_itins; i++) {
		int index = 1+i*5;
		dep_dv = pa_porkchop[index+2];
		sat_dv = pa_porkchop[index+3]+pa_porkchop[index+4]*fb0_pow1;
		tot_dv = dep_dv + sat_dv;
		date = pa_porkchop[index+0];
		dur = pa_porkchop[index+1];

		if(date < min[0]) min[0] = date;
		else if(date > max[0]) max[0] = date;
		if(dur < min[1]) min[1] = dur;
		else if(dur > max[1]) max[1] = dur;
		if(tot_dv < min[2]) min[2] = tot_dv;
		else if(tot_dv > max[2]) max[2] = tot_dv;
		if(dep_dv < min[3]) min[3] = dep_dv;
		else if(dep_dv > max[3]) max[3] = dep_dv;
		if(sat_dv < min[4]) min[4] = sat_dv;
		else if(sat_dv > max[4]) max[4] = sat_dv;
	}

	char string[20];
	date_to_string(convert_JD_date(min[0]), string, 0);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[0]), string);
	sprintf(string, "%.0f", min[1]);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[1]), string);

	for(int i = 2; i < 5; i++) {
		sprintf(string, "%.2f", min[i]);
		gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[i]), string);
	}

	date_to_string(convert_JD_date(max[0]), string, 0);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[0]), string);
	sprintf(string, "%.0f", max[1]);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[1]), string);

	for(int i = 2; i < 5; i++) {
		sprintf(string, "%.2f", max[i]);
		gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[i]), string);
	}
}

void on_change_itin_group_visibility(GtkWidget* widget, gpointer data) {
	struct Group *group = (struct Group *) data;  // Cast data back to group struct
	int visibility = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	group->show_group = visibility;
	on_apply_filter(NULL, NULL);
}

void update_group_overview() {
	int num_cols = 2;

	// Remove grid if exists
	if (grid_pa_groups != NULL && GTK_WIDGET(vp_pa_groups) == gtk_widget_get_parent(grid_pa_groups)) {
		gtk_container_remove(GTK_CONTAINER(vp_pa_groups), grid_pa_groups);
	}


	grid_pa_groups = gtk_grid_new();
	GtkWidget *separator;

//	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
//	gtk_grid_attach(GTK_GRID(grid_pa_groups), separator, 0, 0, num_cols*2+1, 1);

	for (int col = 0; col < num_cols; col++) {
		// Create a horizontal separator line (optional)
		if(col > 0) {
			separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
			gtk_grid_attach(GTK_GRID(grid_pa_groups), separator, col * 2, 1, 1, 1);
		}

		char label_text[20];
		int req_width = -1;
		switch(col) {
			case 0: sprintf(label_text, "Show"); req_width = 50; break;
			case 1: sprintf(label_text, "Itinerary"); req_width = -1; break;
			default:sprintf(label_text, ""); req_width = 50; break;
		}

		// Create a GtkLabel
		GtkWidget *label = gtk_label_new(label_text);
		// width request
		gtk_widget_set_size_request(GTK_WIDGET(label), req_width, -1);
		// set css class
		set_css_class_for_widget(GTK_WIDGET(label), "pag-header");

		// Set the label in the grid at the specified row and column
		gtk_grid_attach(GTK_GRID(grid_pa_groups), label, col*2+1, 1, 1, 1);
		gtk_widget_set_halign(label, GTK_ALIGN_START);
	}

//	separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
//	gtk_grid_attach(GTK_GRID(grid_pa_groups), separator, 0, 2, num_cols*2+1, 1);

//	// Create labels and buttons and add them to the grid
	for (int group_idx = 0; group_idx < pa_num_groups; group_idx++) {
		int row = group_idx*2+3;

		for (int j = 0; j < num_cols; j++) {
			int col = j*2+1;
			char widget_text[5000];
			GtkWidget *widget;
			switch(j) {
				case 0:
					// Create a show group check button
					widget = gtk_check_button_new();
					gtk_widget_set_halign(widget, GTK_ALIGN_CENTER);
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), pa_groups[group_idx].show_group);
					g_signal_connect(widget, "clicked", G_CALLBACK(on_change_itin_group_visibility), &(pa_groups[group_idx]));
					pa_groups[group_idx].cb_pa_show_group = widget;
					break;
				case 1:
					sprintf(widget_text, "");
					struct ItinStep *ptr;
					for(int step_idx = 0; step_idx < pa_groups[group_idx].num_steps; step_idx++) {
						ptr = pa_groups[group_idx].sample_arrival_node;
						for(int k = 0; k < pa_groups[group_idx].num_steps - step_idx - 1; k++) ptr = ptr->prev;
						if(step_idx != 0) sprintf(widget_text, "%s - ", widget_text);
						sprintf(widget_text, "%s%d", widget_text, ptr->body->id);
					}
					// Create itinerary group label
					widget = gtk_label_new(widget_text);
					gtk_widget_set_halign(widget, GTK_ALIGN_START);
					// set css class
					set_css_class_for_widget(GTK_WIDGET(widget), "pag-group-itin");
					break;
				default:
					sprintf(widget_text, "");
					widget = gtk_label_new(widget_text);
					break;
			}

			// Set the label in the grid at the specified row and column
			gtk_grid_attach(GTK_GRID(grid_pa_groups), widget, col, row, 1, 1);

			// Create a horizontal separator line (optional)
			if(col > 0) {
				separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
				gtk_grid_attach(GTK_GRID(grid_pa_groups), separator, col - 1, row, 1, 1);
			}
		}
	}

	gtk_container_add (GTK_CONTAINER (vp_pa_groups), grid_pa_groups);
	gtk_widget_show_all(GTK_WIDGET(vp_pa_groups));
}

// Comparison function for group qsort
int compare_by_count(const void *a, const void *b) {
	const struct Group *groupA = (const struct Group *) a;
	const struct Group *groupB = (const struct Group *) b;

	// Compare based on the count field
	return (groupB->count - groupA->count);
}

void update_itinerary_groups() {
	pa_groups = malloc(200 * sizeof(struct Group));
	pa_num_groups = 0;
	for(int i = 0; i < pa_num_itins; i++) {
		struct ItinStep *ptr, *group_ptr;
		int is_part_of_group = 0;
		for(int j = 0; j < pa_num_groups; j++) {
			ptr = pa_arrivals[i];
			group_ptr = pa_groups[j].sample_arrival_node;
			while(group_ptr != NULL) {
				if(ptr == NULL) break;
				if(ptr->body->id != group_ptr->body->id) break;
				else {
					if(ptr->prev == NULL && group_ptr->prev == NULL) {
						is_part_of_group = 1; break;
					}
				}
				group_ptr = group_ptr->prev;
				ptr = ptr->prev;
			}
			if(is_part_of_group) {
				pa_groups[j].count++;
				break;
			}
		}
		if(!is_part_of_group) {
			pa_groups[pa_num_groups].sample_arrival_node = pa_arrivals[i];
			pa_groups[pa_num_groups].count = 1;
			pa_groups[pa_num_groups].num_steps = 1;
			pa_groups[pa_num_groups].show_group = 1;
			ptr = pa_arrivals[i];
			while(ptr->prev != NULL) {ptr = ptr->prev; pa_groups[pa_num_groups].num_steps++;}
			pa_num_groups++;
		}
	}

	qsort(pa_groups, pa_num_groups, sizeof(struct Group), compare_by_count);


	for(int i = 0; i < pa_num_groups; i++) {
		struct ItinStep *ptr;
		printf("%4d: (%6d)  (%5d)  ", i, pa_groups[i].count, pa_groups[i].num_steps);
		for(int j = 0; j < pa_groups[i].num_steps; j++) {
			ptr = pa_groups[i].sample_arrival_node;
			for(int k = 0; k < pa_groups[i].num_steps - j - 1; k++) ptr = ptr->prev;
			if(j != 0) printf(" -> ");
			printf("%d", ptr->body->id);
		}
		printf("\n");
	}

	printf("num of pa_groups: %d\n", pa_num_groups);
	update_group_overview();
}

void update_best_itin(int num_itins, int fb0_pow1) {
	double *dvs = (double*) malloc(num_itins*sizeof(double));
	for(int i = 0; i < num_itins; i++)  {
		int ind = 1+i*5;
		dvs[i] = pa_porkchop[ind+2]+pa_porkchop[ind+3]+pa_porkchop[ind+4]*fb0_pow1;
	}

	quicksort_porkchop_and_arrivals(dvs, 0, pa_num_itins-1, pa_porkchop, pa_arrivals);

	if(curr_transfer_pa != NULL) free_itinerary(get_first(curr_transfer_pa));
	curr_transfer_pa = create_itin_copy_from_arrival(pa_arrivals[0]);
	current_date_pa = get_last(curr_transfer_pa)->date;

	printf("Best itin idx: %d\n", get_itinerary_group_index(curr_transfer_pa, pa_groups, pa_num_groups));
}

void on_porkchop_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	// reset drawing area
	cairo_rectangle(cr, 0, 0, area_width, area_height);
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
	pa_all_arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	pa_arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < pa_num_deps; i++) store_itineraries_in_array(pa_departures[i], pa_all_arrivals, &index);

	pa_all_porkchop = (double *) malloc((5 * num_itins + 1) * sizeof(double));
	pa_porkchop = (double *) malloc((5 * num_itins + 1) * sizeof(double));
	pa_all_porkchop[0] = 0;
	int fb0_pow1 = pa_last_transfer_type == TF_FLYBY ? 0 : 1;
	for(int i = 0; i < num_itins; i++) {
		create_porkchop_point(pa_all_arrivals[i], &pa_all_porkchop[i * 5 + 1], pa_last_transfer_type == TF_CIRC ? 0 : 1);
		pa_all_porkchop[0] += 5;
	}
	pa_all_num_itins = num_itins;
	pa_num_itins = pa_all_num_itins;
	reset_porkchop_and_arrivals(pa_all_porkchop, pa_porkchop, pa_all_arrivals, pa_arrivals);
	update_itinerary_groups();
	update_best_itin(num_itins, fb0_pow1);
	reset_min_max_feedback(fb0_pow1, pa_num_itins);
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
	update_preview();
}

void on_save_best_itinerary(GtkWidget* widget, gpointer data) {
	struct ItinStep *first = get_first(curr_transfer_pa);
	if(first == NULL) return;


	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
	gint res;

	// Create the file chooser dialog
	dialog = gtk_file_chooser_dialog_new("Save File", NULL, action,
										 "_Cancel", GTK_RESPONSE_CANCEL,
										 "_Save", GTK_RESPONSE_ACCEPT,
										 NULL);

	// Set initial folder
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "./Itineraries");

	// Create a filter for files with the extension .itin
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.itin");
	gtk_file_filter_set_name(filter, ".itin");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	// Run the dialog
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filepath;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filepath = gtk_file_chooser_get_filename(chooser);

		store_single_itinerary_in_bfile(first, filepath);
		g_free(filepath);
	}

	// Destroy the dialog
	gtk_widget_destroy(dialog);
}

void on_last_transfer_type_changed_pa(GtkWidget* widget, gpointer data) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) return;
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "fb") == 0) pa_last_transfer_type = TF_FLYBY;
	else if	(strcmp(name, "capture") == 0) pa_last_transfer_type = TF_CAPTURE;
	else if	(strcmp(name, "circ") == 0) pa_last_transfer_type = TF_CIRC;

	int fb0_pow1 = pa_last_transfer_type == TF_FLYBY ? 0 : 1;

	for(int i = 0; i < pa_num_itins; i++) {
		create_porkchop_point(pa_arrivals[i], &pa_porkchop[i * 5 + 1], pa_last_transfer_type == TF_CIRC ? 0 : 1);
	}
	for(int i = 0; i < pa_all_num_itins; i++) {
		create_porkchop_point(pa_all_arrivals[i], &pa_all_porkchop[i * 5 + 1], pa_last_transfer_type == TF_CIRC ? 0 : 1);
	}
	update_best_itin(pa_num_itins, fb0_pow1);
	update_porkchop_drawing_area();
	update_preview();
}

void update_pa() {
	int fb0_pow1 = pa_last_transfer_type == TF_FLYBY ? 0 : 1;
	update_best_itin(pa_num_itins, fb0_pow1);
	update_porkchop_drawing_area();
	update_preview();
//	reset_min_max_feedback(fb0_pow1, pa_num_itins);
}

void on_apply_filter(GtkWidget* widget, gpointer data) {
	double min[5], max[5];
	char *string;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_min_feedback[0]));
	min[0] = convert_date_JD(date_from_string(string))-0.9;	// rounding imprecision in filter entry field
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_max_feedback[0]));
	max[0] = convert_date_JD(date_from_string(string))+0.9;	// rounding imprecision in filter entry field
	for(int i = 1; i < 5; i++) {
		string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_min_feedback[i]));
		min[i] = strtod(string, NULL)-0.01;	// rounding imprecision in filter entry field
		string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_max_feedback[i]));
		max[i] = strtod(string, NULL)+0.01;	// rounding imprecision in filter entry field
	}

	int init_num_itins = pa_num_itins;

	int fb0_pow1 = pa_last_transfer_type == TF_FLYBY ? 0 : 1;
	reset_porkchop_and_arrivals(pa_all_porkchop, pa_porkchop, pa_all_arrivals, pa_arrivals);
	pa_num_itins = filter_porkchop_arrivals_depdate(pa_porkchop, pa_arrivals, min[0], max[0]);
	pa_num_itins = filter_porkchop_arrivals_dur(pa_porkchop, pa_arrivals, min[1], max[1]);
	pa_num_itins = filter_porkchop_arrivals_totdv(pa_porkchop, pa_arrivals, min[2], max[2], fb0_pow1);
	pa_num_itins = filter_porkchop_arrivals_depdv(pa_porkchop, pa_arrivals, min[3], max[3]);
	pa_num_itins = filter_porkchop_arrivals_satdv(pa_porkchop, pa_arrivals, min[4], max[4], fb0_pow1);
	pa_num_itins = filter_porkchop_arrivals_groups(pa_porkchop, pa_arrivals, pa_groups, pa_num_groups);

	printf("Filtered %d Itineraries (%d left)\n", init_num_itins-pa_num_itins, pa_num_itins);

	update_pa();
}

void on_reset_filter(GtkWidget* widget, gpointer data) {
	int fb0_pow1 = pa_last_transfer_type == TF_FLYBY ? 0 : 1;
	reset_min_max_feedback(fb0_pow1, pa_num_itins);
}

void on_reset_porkchop(GtkWidget* widget, gpointer data) {
	if(pa_all_porkchop == NULL) return;
	for(int i = 0; i < pa_num_groups; i++) {
		pa_groups[i].show_group = 1;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pa_groups[i].cb_pa_show_group), 1);
	}
	reset_porkchop_and_arrivals(pa_all_porkchop, pa_porkchop, pa_all_arrivals, pa_arrivals);
	pa_num_itins = pa_all_num_itins;
	update_pa();
}

void on_prev_transfer_pa(GtkWidget* widget, gpointer data) {
	if(curr_transfer_pa == NULL) return;
	if(curr_transfer_pa->prev != NULL) curr_transfer_pa = curr_transfer_pa->prev;
	current_date_pa = curr_transfer_pa->date;
	update_preview();
}

void on_next_transfer_pa(GtkWidget* widget, gpointer data) {
	if(curr_transfer_pa == NULL) return;
	if(curr_transfer_pa->next != NULL) curr_transfer_pa = curr_transfer_pa->next[0];
	current_date_pa = curr_transfer_pa->date;
	update_preview();
}

void on_switch_steps_groups(GtkWidget* widget, gpointer data) {
	if(strcmp(gtk_stack_get_visible_child_name(GTK_STACK(st_pa_step_group_selector)), "page0") == 0)
		gtk_stack_set_visible_child_name(GTK_STACK(st_pa_step_group_selector), "page1");
	else
		gtk_stack_set_visible_child_name(GTK_STACK(st_pa_step_group_selector), "page0");
}