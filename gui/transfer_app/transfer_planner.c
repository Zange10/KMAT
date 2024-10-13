#include "transfer_planner.h"
#include "tools/analytic_geometry.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "gui/drawing.h"
#include "gui/transfer_app.h"
#include "tools/gmat_interface.h"

#include <string.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <math.h>


struct ItinStep *curr_transfer_tp;

GObject *da_tp;
GObject *lb_tp_date;
GObject *tb_tp_tfdate;
GObject *bt_tp_tfbody;
GObject *lb_tp_transfer_dv;
GObject *lb_tp_total_dv;
GObject *lb_tp_periapsis;
GObject *transfer_panel_tp;
gboolean body_show_status_tp[9];
double current_date_tp;

enum LastTransferType tp_last_transfer_type;


void init_transfer_planner(GtkBuilder *builder) {
	struct Date date = {1977, 8, 20, 0, 0, 0};
	current_date_tp = convert_date_JD(date);
	for(int i = 0; i < 9; i++) body_show_status_tp[i] = 0;
	remove_all_transfers();
	tp_last_transfer_type = TF_FLYBY;

	tb_tp_tfdate = gtk_builder_get_object(builder, "tb_tp_tfdate");
	bt_tp_tfbody = gtk_builder_get_object(builder, "bt_tp_change_tf_body");
	lb_tp_transfer_dv = gtk_builder_get_object(builder, "lb_tp_transfer_dv");
	lb_tp_total_dv = gtk_builder_get_object(builder, "lb_tp_total_dv");
	lb_tp_periapsis = gtk_builder_get_object(builder, "lb_tp_periapsis");
	transfer_panel_tp = gtk_builder_get_object(builder, "transfer_panel");
	lb_tp_date = gtk_builder_get_object(builder, "lb_tp_date");
	da_tp = gtk_builder_get_object(builder, "da_tp");

	update_date_label();
	update_transfer_panel();
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
		struct ItinStep *temp_transfer = get_first(curr_transfer_tp);
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
		if(tp_last_transfer_type == TF_FLYBY) return 0;
		double vinf = vector_mag(subtract_vectors(step->v_arr, step->v_body));
		if(tp_last_transfer_type == TF_CAPTURE) return dv_capture(step->body, step->body->atmo_alt + 100e3, vinf);
		else if(tp_last_transfer_type == TF_CIRC) return dv_circ(step->body, step->body->atmo_alt + 100e3, vinf);
	}
	return 0;
}

double calc_total_dv() {
	if(curr_transfer_tp == NULL || !is_valid_itinerary(get_last(curr_transfer_tp))) return 0;
	double dv = 0;
	struct ItinStep *step = get_last(curr_transfer_tp);
	while(step != NULL) {
		dv += calc_step_dv(step);
		step = step->prev;
	}
	return dv;
}

double calc_periapsis_height_tp() {
	if(curr_transfer_tp->body == NULL) return -1e20;
	if(curr_transfer_tp->next == NULL || curr_transfer_tp->prev == NULL) return -1e20;
	double rp = get_flyby_periapsis(curr_transfer_tp->v_arr, curr_transfer_tp->next[0]->v_dep, curr_transfer_tp->v_body, curr_transfer_tp->body);
	return (rp-curr_transfer_tp->body->radius)*1e-3;
}

void update_itinerary() {
	update_itin_body_osvs(get_first(curr_transfer_tp), get_body_ephems());
	calc_itin_v_vectors_from_dates_and_r(get_first(curr_transfer_tp));
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
	free_itinerary(get_first(curr_transfer_tp));
	curr_transfer_tp = NULL;
}

void update() {
	update_date_label();
	update_transfer_panel();
	gtk_widget_queue_draw(GTK_WIDGET(da_tp));
}

void update_date_label() {
	char date_string[10];
	date_to_string(convert_JD_date(current_date_tp), date_string, 0);
	gtk_label_set_text(GTK_LABEL(lb_tp_date), date_string);
}

void update_transfer_panel() {
	if(curr_transfer_tp == NULL) {
		gtk_button_set_label(GTK_BUTTON(tb_tp_tfdate), "0000-00-00");
		gtk_button_set_label(GTK_BUTTON(bt_tp_tfbody), "Planet");
	} else {
		struct Date date = convert_JD_date(curr_transfer_tp->date);
		char date_string[10];
		date_to_string(date, date_string, 0);
		gtk_button_set_label(GTK_BUTTON(tb_tp_tfdate), date_string);
		if(curr_transfer_tp->body != NULL) gtk_button_set_label(GTK_BUTTON(bt_tp_tfbody), curr_transfer_tp->body->name);
		else gtk_button_set_label(GTK_BUTTON(bt_tp_tfbody), "Deep-Space Man");
		char s_dv[20];
		sprintf(s_dv, "%6.0f m/s", calc_step_dv(curr_transfer_tp));
		gtk_label_set_label(GTK_LABEL(lb_tp_transfer_dv), s_dv);
		sprintf(s_dv, "%6.0f m/s", calc_total_dv());
		gtk_label_set_label(GTK_LABEL(lb_tp_total_dv), s_dv);
		double h = calc_periapsis_height_tp();
		if(curr_transfer_tp->body != NULL && h > -curr_transfer_tp->body->radius*1e-3)
			sprintf(s_dv, "%.0f km", h);
		else sprintf(s_dv, "- km");
		gtk_label_set_label(GTK_LABEL(lb_tp_periapsis), s_dv);
	}

}


void on_body_toggle(GtkWidget* widget, gpointer data) {
	int id = (int) gtk_widget_get_name(widget)[0] - 48;	// char to int
	body_show_status_tp[id - 1] = body_show_status_tp[id - 1] ? 0 : 1;
	gtk_widget_queue_draw(GTK_WIDGET(da_tp));
}


void on_change_date(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "+1Y") == 0) current_date_tp = jd_change_date(current_date_tp, 1, 0, 0);
	else if	(strcmp(name, "+1M") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 1, 0);
	else if	(strcmp(name, "+1D") == 0) current_date_tp++;
	else if	(strcmp(name, "-1Y") == 0) current_date_tp = jd_change_date(current_date_tp, -1, 0, 0);
	else if	(strcmp(name, "-1M") == 0) current_date_tp = jd_change_date(current_date_tp, 0, -1, 0);
	else if	(strcmp(name, "-1D") == 0) current_date_tp--;

	if(curr_transfer_tp != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate))) {
		if(curr_transfer_tp->prev != NULL && current_date_tp <= curr_transfer_tp->prev->date) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
			return;
		}
		curr_transfer_tp->date = current_date_tp;
		sort_transfer_dates(get_first(curr_transfer_tp));
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
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	current_date_tp = curr_transfer_tp->date;
	update();
}

void on_next_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	if(curr_transfer_tp->next != NULL) curr_transfer_tp = curr_transfer_tp->next[0];
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	current_date_tp = curr_transfer_tp->date;
	update();
}

void on_transfer_body_change(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	gtk_stack_set_visible_child_name(GTK_STACK(transfer_panel_tp), "page1");
	update_itinerary();
}

void on_last_transfer_type_changed_tp(GtkWidget* widget, gpointer data) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) return;
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "fb") == 0) tp_last_transfer_type = TF_FLYBY;
	else if	(strcmp(name, "capture") == 0) tp_last_transfer_type = TF_CAPTURE;
	else if	(strcmp(name, "circ") == 0) tp_last_transfer_type = TF_CIRC;
	update_transfer_panel();
}

void on_toggle_transfer_date_lock(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp->body == NULL) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	if(curr_transfer_tp != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate)))
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
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
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
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
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
	struct ItinStep *itin_copy = create_itin_copy(get_first(curr_transfer_tp));
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
		struct ItinStep *itin = get_first(curr_transfer_tp);
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
	struct ItinStep *first = get_first(curr_transfer_tp);
	if(first == NULL || !is_valid_itinerary(get_last(curr_transfer_tp))) return;


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

		if(curr_transfer_tp != NULL) free_itinerary(get_first(curr_transfer_tp));
		curr_transfer_tp = load_single_itinerary_from_bfile(filepath);
		current_date_tp = curr_transfer_tp->date;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
		update_itinerary();
		g_free(filepath);
	} else {
		// Destroy the dialog
		gtk_widget_destroy(dialog);
		return;
	}

	struct ItinStep *step2pr = get_first(curr_transfer_tp);
	struct DepArrHyperbolaParams dep_hyp_params = get_dep_hyperbola_params(step2pr->next[0]->v_dep, step2pr->v_body,
																		   step2pr->body, 200e3);
	printf("\nDeparture Hyperbola %s\n"
		   "Date: %f\n"
		   "OutgoingRadPer: %f km\n"
		   "OutgoingC3Energy: %f km²/s²\n"
		   "OutgoingRHA: %f°\n"
		   "OutgoingDHA: %f°\n"
		   "OutgoingBVAZI: -°\n"
		   "TA: 0.0°\n",
		   step2pr->body->name, step2pr->date, dep_hyp_params.r_pe/1000, dep_hyp_params.c3_energy/1e6,
		   rad2deg(dep_hyp_params.bplane_angle), rad2deg(dep_hyp_params.decl));
	
	step2pr = step2pr->next[0];
	while(step2pr->num_next_nodes != 0) {
		if(step2pr->body != NULL) {
			struct Vector v_arr = step2pr->v_arr;
			struct Vector v_dep = step2pr->next[0]->v_dep;
			struct Vector v_body = step2pr->v_body;
			double rp = get_flyby_periapsis(v_arr, v_dep, v_body, step2pr->body);
			double incl = get_flyby_inclination(v_arr, v_dep, v_body);

			struct FlybyHyperbolaParams hyp_params = get_hyperbola_params(step2pr->v_arr, step2pr->next[0]->v_dep,
																		  step2pr->v_body, step2pr->body,
																		  rp - step2pr->body->radius);
			double dt_in_days = step2pr->date - step2pr->prev->date;

			printf("\nFly-by Hyperbola %s (Travel Time: %.2f days)\n"
				   "Date: %f\n"
				   "RadPer: %f km\n"
				   "Inclination: %f°\n"
				   "C3Energy: %f km²/s²\n"
				   "IncomingRHA: %f°\n"
				   "IncomingDHA: %f°\n"
				   "IncomingBVAZI: %f°\n"
				   "OutgoingRHA: %f°\n"
				   "OutgoingDHA: %f°\n"
				   "OutgoingBVAZI: %f°\n"
				   "TA: 0.0°\n",
				   step2pr->body->name, dt_in_days, step2pr->date, hyp_params.dep_hyp.r_pe / 1000, rad2deg(incl),
				   hyp_params.dep_hyp.c3_energy / 1e6,
				   rad2deg(hyp_params.arr_hyp.bplane_angle), rad2deg(hyp_params.arr_hyp.decl),
				   rad2deg(hyp_params.arr_hyp.bvazi),
				   rad2deg(hyp_params.dep_hyp.bplane_angle), rad2deg(hyp_params.dep_hyp.decl),
				   rad2deg(hyp_params.dep_hyp.bvazi));
			step2pr = step2pr->next[0];
		} else {
			double dt_in_days = step2pr->date - step2pr->prev->date;
			double dist_to_sun = vector_mag(step2pr->r);

			struct Vector orbit_prograde = step2pr->v_arr;
			struct Vector orbit_normal = cross_product(step2pr->r, step2pr->v_arr);
			struct Vector orbit_radialin = cross_product(orbit_normal, step2pr->v_arr);
			struct Vector dv_vec = subtract_vectors(step2pr->next[0]->v_dep, step2pr->v_arr);

			// dv vector in S/C coordinate system (prograde, radial in, normal) (sign it if projected vector more than 90° from target vector / pointing in opposite direction)
			struct Vector dv_vec_sc = {
					vector_mag(proj_vec_vec(dv_vec, orbit_prograde)) * (angle_vec_vec(proj_vec_vec(dv_vec, orbit_prograde), orbit_prograde) < M_PI/2 ? 1 : -1),
					vector_mag(proj_vec_vec(dv_vec, orbit_radialin)) * (angle_vec_vec(proj_vec_vec(dv_vec, orbit_radialin), orbit_radialin) < M_PI/2 ? 1 : -1),
					vector_mag(proj_vec_vec(dv_vec, orbit_normal)) * (angle_vec_vec(proj_vec_vec(dv_vec, orbit_normal), orbit_normal) < M_PI/2 ? 1 : -1)
			};

			printf("\nDeep Space Maneuver (Travel Time: %.2f days)\n"
				   "Date: %f\n"
				   "Distance to the Sun: %.3f AU\n"
				   "Dv Prograde: %f m/s\n"
				   "Dv Radial: %f m/s\n"
				   "Dv Normal: %f m/s\n"
				   "Total: %f m/s\n",
				   dt_in_days, step2pr->date, dist_to_sun / 1.495978707e11, dv_vec_sc.x, dv_vec_sc.y, dv_vec_sc.z, vector_mag(dv_vec_sc));
			step2pr = step2pr->next[0];
		}
	}
	
	double rp = 100000e3;
	double dt_in_days = step2pr->date - step2pr->prev->date;
	struct DepArrHyperbolaParams arr_hyp_params = get_dep_hyperbola_params(step2pr->v_arr, step2pr->v_body, step2pr->body, rp - step2pr->body->radius);
	arr_hyp_params.decl *= -1;
	arr_hyp_params.bplane_angle = pi_norm(M_PI + arr_hyp_params.bplane_angle);
	printf("\nArrival Hyperbola %s (Travel Time: %.2f days)\n"
		   "Date: %f\n"
		   "IncomingRadPer: %f km\n"
		   "IncomingC3Energy: %f km²/s²\n"
		   "IncomingRHA: %f°\n"
		   "IncomingDHA: %f°\n"
		   "IncomingBVAZI: -°\n"
		   "TA: 0.0°\n",
		   step2pr->body->name, dt_in_days, step2pr->date, arr_hyp_params.r_pe/1000, arr_hyp_params.c3_energy/1e6,
		   rad2deg(arr_hyp_params.bplane_angle), rad2deg(arr_hyp_params.decl));

	// Destroy the dialog
	gtk_widget_destroy(dialog);
}


void on_create_gmat_script() {
	if(curr_transfer_tp != NULL) write_gmat_script(curr_transfer_tp, "transfer.script");
}
