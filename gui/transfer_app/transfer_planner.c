#include "transfer_planner.h"
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "gui/drawing.h"
#include "gui/transfer_app.h"

#include <string.h>
#include <gtk/gtk.h>
#include <locale.h>


struct ItinStep *curr_transfer_tp;

GObject *drawing_area_tp;
GObject *lb_date_tp;
GObject *tb_tfdate_tp;
GObject *bt_tfbody_tp;
GObject *lb_transfer_dv_tp;
GObject *lb_total_dv_tp;
GObject *transfer_panel_tp;
gboolean body_show_status_tp[9];
double current_date_tp;

enum LastTransferType last_transfer_type_tp;


void init_transfer_planner(GtkBuilder *builder) {
	tb_tfdate_tp = gtk_builder_get_object(builder, "tb_tfdate");
	bt_tfbody_tp = gtk_builder_get_object(builder, "bt_change_tf_body");
	lb_transfer_dv_tp = gtk_builder_get_object(builder, "lb_transfer_dv");
	lb_total_dv_tp = gtk_builder_get_object(builder, "lb_total_dv");
	transfer_panel_tp = gtk_builder_get_object(builder, "transfer_panel");
	lb_date_tp = gtk_builder_get_object(builder, "lb_date");
	drawing_area_tp = gtk_builder_get_object(builder, "draw_transfer_planner");

	struct Date date = {1977, 8, 20, 0, 0, 0};
	current_date_tp = convert_date_JD(date);
	for(int i = 0; i < 9; i++) body_show_status_tp[i] = 0;
	remove_all_transfers();
	last_transfer_type_tp = TF_FLYBY;

	update_date_label();
	update_transfer_panel();
}



struct ItinStep * get_first() {
	if(curr_transfer_tp == NULL) return NULL;
	struct ItinStep *tf = curr_transfer_tp;
	while(tf->prev != NULL) tf = tf->prev;
	return tf;
}

struct ItinStep * get_last() {
	if(curr_transfer_tp == NULL) return NULL;
	struct ItinStep *tf = curr_transfer_tp;
	while(tf->next != NULL) tf = tf->next[0];
	return tf;
}


void on_transfer_planner_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
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
	for(int i = 0; i < 9; i++) if(body_show_status_tp[i]) highest_id = i + 1;
	double scale = calc_scale(area_width, area_height, highest_id);

	// Sun
	set_cairo_body_color(cr, 0);
	draw_body(cr, center, 0, vec(0,0,0));
	cairo_fill(cr);

	// Planets
	for(int i = 0; i < 9; i++) {
		if(body_show_status_tp[i]) {
			int id = i+1;
			set_cairo_body_color(cr, id);
			struct OSV osv = osv_from_ephem(get_body_ephems()[i], current_date_tp, SUN());
			draw_body(cr, center, scale, osv.r);
			draw_orbit(cr, center, scale, osv.r, osv.v, SUN());
		}
	}

	// Transfers
	if(curr_transfer_tp != NULL) {
		struct ItinStep *temp_transfer = get_first();
		while(temp_transfer != NULL) {
			if(temp_transfer->body != NULL) {
				int id = temp_transfer->body->id;
				set_cairo_body_color(cr, id);
				draw_transfer_point(cr, center, scale, temp_transfer->r);
				// skip not working or draw working double swing-by
			} else if(temp_transfer->body == NULL && temp_transfer->v_body.x == 1)
				draw_transfer_point(cr, center, scale, temp_transfer->r);
			if(temp_transfer->prev != NULL) draw_trajectory(cr, center, scale, temp_transfer);
			temp_transfer = temp_transfer->next != NULL ? temp_transfer->next[0] : NULL;
		}
	}

	update_transfer_panel();	// Seems redundant, but necessary for updating dv numbers
}

double calc_step_dv(struct ItinStep *step) {
	if(step == NULL || (step->prev == NULL && step->next == NULL)) return 0;
	if(step->body == NULL) {
		if(step->next == NULL || step->next[0]->next == NULL || step->prev == NULL)
			return 0;
		if(step->v_body.x == 0) return 0;
		return vector_mag(subtract_vectors(step->v_arr, step->next[0]->v_dep));
	} else if(step->prev == NULL) {
		double vinf = vector_mag(subtract_vectors(step->next[0]->v_dep, step->v_body));
		return dv_circ(step->body, step->body->atmo_alt+100e3, vinf);
	} else if(step->next == NULL) {
		if(last_transfer_type_tp == TF_FLYBY) return 0;
		double vinf = vector_mag(subtract_vectors(step->v_arr, step->v_body));
		if(last_transfer_type_tp == TF_CAPTURE) return dv_capture(step->body, step->body->atmo_alt + 100e3, vinf);
		else if(last_transfer_type_tp == TF_CIRC) return dv_circ(step->body, step->body->atmo_alt + 100e3, vinf);
	}
	return 0;
}

double calc_total_dv() {
	if(curr_transfer_tp == NULL || !is_valid_itinerary(get_last())) return 0;
	double dv = 0;
	struct ItinStep *step = get_last();
	while(step != NULL) {
		dv += calc_step_dv(step);
		step = step->prev;
	}
	return dv;
}

void update_itinerary() {
	update_itin_body_osvs(get_first(), get_body_ephems());
	calc_itin_v_vectors_from_dates_and_r(get_first());
	update();
}

void sort_transfer_dates(struct ItinStep *tf) {
	if(tf == NULL) return;
	while(tf->next != NULL) {
		if(tf->next[0]->date <= tf->date) tf->next[0]->date = tf->date+1;
		tf = tf->next[0];
	}
	update_itinerary();
}

void remove_all_transfers() {
	if(curr_transfer_tp == NULL) return;
	free_itinerary(get_first());
	curr_transfer_tp = NULL;
}

void update() {
	update_date_label();
	update_transfer_panel();
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area_tp));
}

void update_date_label() {
	char date_string[10];
	date_to_string(convert_JD_date(current_date_tp), date_string, 0);
	gtk_label_set_text(GTK_LABEL(lb_date_tp), date_string);
}

void update_transfer_panel() {
	if(curr_transfer_tp == NULL) {
		gtk_button_set_label(GTK_BUTTON(tb_tfdate_tp), "0000-00-00");
		gtk_button_set_label(GTK_BUTTON(bt_tfbody_tp), "Planet");
	} else {
		struct Date date = convert_JD_date(curr_transfer_tp->date);
		char date_string[10];
		date_to_string(date, date_string, 0);
		gtk_button_set_label(GTK_BUTTON(tb_tfdate_tp), date_string);
		if(curr_transfer_tp->body != NULL) gtk_button_set_label(GTK_BUTTON(bt_tfbody_tp), curr_transfer_tp->body->name);
		else gtk_button_set_label(GTK_BUTTON(bt_tfbody_tp), "Deep-Space Man");
		char s_dv[20];
		sprintf(s_dv, "%6.0f m/s", calc_step_dv(curr_transfer_tp));
		gtk_label_set_label(GTK_LABEL(lb_transfer_dv_tp), s_dv);
		sprintf(s_dv, "%6.0f m/s", calc_total_dv());
		gtk_label_set_label(GTK_LABEL(lb_total_dv_tp), s_dv);
	}
}


void on_body_toggle(GtkWidget* widget, gpointer data) {
	int id = (int) gtk_widget_get_name(widget)[0] - 48;	// char to int
	body_show_status_tp[id - 1] = body_show_status_tp[id - 1] ? 0 : 1;
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area_tp));
}


void on_change_date(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "+1Y") == 0) current_date_tp = jd_change_date(current_date_tp, 1, 0, 0);
	else if	(strcmp(name, "+1M") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 1, 0);
	else if	(strcmp(name, "+1D") == 0) current_date_tp++;
	else if	(strcmp(name, "-1Y") == 0) current_date_tp = jd_change_date(current_date_tp, -1, 0, 0);
	else if	(strcmp(name, "-1M") == 0) current_date_tp = jd_change_date(current_date_tp, 0, -1, 0);
	else if	(strcmp(name, "-1D") == 0) current_date_tp--;

	if(curr_transfer_tp != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp))) {
		if(curr_transfer_tp->prev != NULL && current_date_tp <= curr_transfer_tp->prev->date) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp), 0);
			return;
		}
		curr_transfer_tp->date = current_date_tp;
		sort_transfer_dates(get_first());
	}

	update_itinerary();
}


void on_year_select(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	int year = atoi(name);
	struct Date date = {year, 1,1};
	current_date_tp = convert_date_JD(date);
	update_itinerary();
}



void on_prev_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	if(curr_transfer_tp->prev != NULL) curr_transfer_tp = curr_transfer_tp->prev;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp), 0);
	current_date_tp = curr_transfer_tp->date;
	update();
}

void on_next_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	if(curr_transfer_tp->next != NULL) curr_transfer_tp = curr_transfer_tp->next[0];
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp), 0);
	current_date_tp = curr_transfer_tp->date;
	update();
}

void on_transfer_body_change(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	gtk_stack_set_visible_child_name(GTK_STACK(transfer_panel_tp), "page1");
	update_itinerary();
}

void on_last_transfer_type_changed(GtkWidget* widget, gpointer data) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) return;
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "fb") == 0) last_transfer_type_tp = TF_FLYBY;
	else if	(strcmp(name, "capture") == 0) last_transfer_type_tp = TF_CAPTURE;
	else if	(strcmp(name, "circ") == 0) last_transfer_type_tp = TF_CIRC;
	update_transfer_panel();
}

void on_toggle_transfer_date_lock(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp->body == NULL) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp), 0);
	if(curr_transfer_tp != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp)))
		current_date_tp = curr_transfer_tp->date;
	update_itinerary();
}

void on_goto_transfer_date(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	current_date_tp = curr_transfer_tp->date;
	update_itinerary();
}

void on_transfer_body_select(GtkWidget* widget, gpointer data) {
	int id = (int) gtk_widget_get_name(widget)[0] - 48;	// char to int
	if(id == 0) {
		curr_transfer_tp->body = NULL;
	} else {
		struct Body *body = get_body_from_id(id);
		curr_transfer_tp->body = body;
	}
	gtk_stack_set_visible_child_name(GTK_STACK(transfer_panel_tp), "page0");
	update_itinerary();
}

void on_add_transfer(GtkWidget* widget, gpointer data) {
	struct ItinStep *new_transfer = (struct ItinStep *) malloc(sizeof(struct ItinStep));
	new_transfer->body = EARTH();
	new_transfer->prev = NULL;
	new_transfer->next = NULL;
	new_transfer->num_next_nodes = 0;

	struct ItinStep *next = (curr_transfer_tp != NULL && curr_transfer_tp->next != NULL) ? curr_transfer_tp->next[0] : NULL;

	// date
	new_transfer->date = current_date_tp;
	if(curr_transfer_tp != NULL) {
		if(current_date_tp <= curr_transfer_tp->date) new_transfer->date = curr_transfer_tp->date + 1;
		if(next != NULL) {
			if(next->date - curr_transfer_tp->date < 2) {
				free(new_transfer);
				return;    // no space between this and next transfer...
			}
			if(new_transfer->date >= next->date) new_transfer->date = next->date - 1;
		}
	}

	// next and prev pointers
	if(curr_transfer_tp != NULL) {
		if(next != NULL) {
			next->prev = new_transfer;
			new_transfer->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
			new_transfer->next[0] = next;
			new_transfer->num_next_nodes = 1;
		} else {
			curr_transfer_tp->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
			curr_transfer_tp->num_next_nodes = 1;
		}
		new_transfer->prev = curr_transfer_tp;
		curr_transfer_tp->next[0] = new_transfer;
	}

	// Location and Velocity vectors
	update_itinerary();

	curr_transfer_tp = new_transfer;
	current_date_tp = curr_transfer_tp->date;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp), 0);
	update_itinerary();
}

void on_remove_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	struct ItinStep *rem_transfer = curr_transfer_tp;
	if(curr_transfer_tp->next != NULL) curr_transfer_tp->next[0]->prev = curr_transfer_tp->prev;
	if(curr_transfer_tp->prev != NULL) {
		if(curr_transfer_tp->next != NULL) {
			curr_transfer_tp->prev->next[0] = curr_transfer_tp->next[0];
		} else {
			free(curr_transfer_tp->prev->next);
			curr_transfer_tp->prev->next = NULL;
			curr_transfer_tp->prev->num_next_nodes = 0;
		}
	}

	if(curr_transfer_tp->prev != NULL) curr_transfer_tp = curr_transfer_tp->prev;
	else if(curr_transfer_tp->next != NULL) curr_transfer_tp = curr_transfer_tp->next[0];
	else curr_transfer_tp = NULL;

	free(rem_transfer->next);
	free(rem_transfer);
	if(curr_transfer_tp != NULL) current_date_tp = curr_transfer_tp->date;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp), 0);
	update();
}

int find_closest_transfer(struct ItinStep *step) {
	if(step == NULL || step->prev == NULL || step->prev->prev == NULL) return 0;
	struct ItinStep *prev = step->prev;
	struct ItinStep *temp = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	copy_step_body_vectors_and_date(prev, temp);
	temp->next = NULL;
	temp->prev = NULL;
	temp->num_next_nodes = 0;
	find_viable_flybys(temp, get_body_ephems()[step->body->id-1], step->body, 86400, 86400*365.25*50);

	if(temp->next != NULL) {
		struct ItinStep *new_step = temp->next[0];
		copy_step_body_vectors_and_date(new_step, step);
		for(int i = 0; i < temp->num_next_nodes; i++) free(temp->next[i]);
		free(temp->next);
		current_date_tp = step->date;
		free(temp);
		sort_transfer_dates(step);
		return 1;
	} else {
		free(temp);
		return 0;
	}
}

void on_find_closest_transfer(GtkWidget* widget, gpointer data) {
	int success = find_closest_transfer(curr_transfer_tp);
	if(success) update_itinerary();
}

void on_find_itinerary(GtkWidget* widget, gpointer data) {
	struct ItinStep *itin_copy = create_itin_copy(get_first());
	while(itin_copy->prev != NULL) {
		if(itin_copy->prev->body == NULL) return;	// double swing-by not implemented
		itin_copy = itin_copy->prev;
	}
	if(itin_copy == NULL || itin_copy->next == NULL || itin_copy->next[0]->next == NULL) return;

	itin_copy = itin_copy->next[0];
	int status = 1;

	while(itin_copy->next != NULL) {
		itin_copy = itin_copy->next[0];
		status = find_closest_transfer(itin_copy);
		if(!status) break;
	}

	if(status) {
		while(itin_copy->prev != NULL) itin_copy = itin_copy->prev;
		struct ItinStep *itin = get_first();
		while(itin != NULL) {
			copy_step_body_vectors_and_date(itin_copy, itin);
			if(itin->next != NULL) {
				itin = itin->next[0];
				itin_copy = itin_copy->next[0];
			} else {
				itin = NULL;
			}
		}
		update_itinerary();
	}

	free_itinerary(itin_copy);
}

void on_save_itinerary(GtkWidget* widget, gpointer data) {
	struct ItinStep *first = get_first();
	if(first == NULL || !is_valid_itinerary(get_last())) return;


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

void on_load_itinerary(GtkWidget* widget, gpointer data) {
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
	gtk_file_filter_add_pattern(filter, "*.itin");
	gtk_file_filter_set_name(filter, ".itin");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	// Run the dialog
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		char *filepath;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		filepath = gtk_file_chooser_get_filename(chooser);

		if(curr_transfer_tp != NULL) free_itinerary(get_first());
		curr_transfer_tp = load_single_itinerary_from_bfile(filepath);
		current_date_tp = curr_transfer_tp->date;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate_tp), 0);
		update_itinerary();
		g_free(filepath);
	}

	// Destroy the dialog
	gtk_widget_destroy(dialog);
}