#include "transfer_planner.h"
#include "gui/drawing.h"
#include "gui/gui_manager.h"
#include "gui/settings.h"
#include "tools/gmat_interface.h"
#include "gui/css_loader.h"
#include "tools/file_io.h"
#include "gui/info_win_manager.h"

#include <string.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <math.h>


struct ItinStep *curr_transfer_tp;

CelestSystem *tp_system;

Camera *tp_system_camera;

GObject *da_tp;
GObject *cb_tp_system;
GObject *cb_tp_central_body;
GObject *cb_tp_tfbody;
GObject *lb_tp_date;
GObject *tb_tp_tfdate;
GObject *bt_tp_10y6hp;
GObject *bt_tp_10y6hm;
GObject *bt_tp_1y1hp;
GObject *bt_tp_1y1hm;
GObject *bt_tp_1m30d10minp;
GObject *bt_tp_1m30d10minm;
GObject *bt_tp_1d1minp;
GObject *bt_tp_1d1minm;
GObject *lb_tp_transfer_dv;
GObject *lb_tp_total_dv;
GObject *lb_tp_param_labels;
GObject *lb_tp_param_values;
GObject *vp_tp_bodies;
GtkWidget *grid_tp_bodies;

gboolean *body_show_status_tp, fix_tfbody;
double current_date_tp;

enum LastTransferType tp_last_transfer_type;

double tp_dep_periapsis = 50e3;
double tp_arr_periapsis = 50e3;

void tp_update_bodies();
void remove_all_transfers();

void update_tp_system_view();
void on_tp_screen_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer *ptr);
void on_tp_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr);
void on_tp_screen_mouse_move(GtkWidget *widget, GdkEventButton *event, gpointer *ptr);


void init_transfer_planner(GtkBuilder *builder) {
	struct Datetime date = {1950, 1, 1, 0, 0, 0};
	current_date_tp = convert_date_JD(date);

	remove_all_transfers();
	tp_last_transfer_type = TF_FLYBY;

	cb_tp_system = gtk_builder_get_object(builder, "cb_tp_system");
	cb_tp_central_body = gtk_builder_get_object(builder, "cb_tp_central_body");
	cb_tp_tfbody = gtk_builder_get_object(builder, "cb_tp_tfbody");
	tb_tp_tfdate = gtk_builder_get_object(builder, "tb_tp_tfdate");
	bt_tp_10y6hp = gtk_builder_get_object(builder, "bt_tp_10y6hp");
	bt_tp_10y6hm = gtk_builder_get_object(builder, "bt_tp_10y6hm");
	bt_tp_1y1hp = gtk_builder_get_object(builder, "bt_tp_1y1hp");
	bt_tp_1y1hm = gtk_builder_get_object(builder, "bt_tp_1y1hm");
	bt_tp_1m30d10minp = gtk_builder_get_object(builder, "bt_tp_1m30d10minp");
	bt_tp_1m30d10minm = gtk_builder_get_object(builder, "bt_tp_1m30d10minm");
	bt_tp_1d1minp = gtk_builder_get_object(builder, "bt_tp_1d1minp");
	bt_tp_1d1minm = gtk_builder_get_object(builder, "bt_tp_1d1minm");
	lb_tp_transfer_dv = gtk_builder_get_object(builder, "lb_tp_transfer_dv");
	lb_tp_total_dv = gtk_builder_get_object(builder, "lb_tp_total_dv");
	lb_tp_param_labels = gtk_builder_get_object(builder, "lb_tp_param_labels");
	lb_tp_param_values = gtk_builder_get_object(builder, "lb_tp_param_values");
	lb_tp_date = gtk_builder_get_object(builder, "lb_tp_date");
	da_tp = gtk_builder_get_object(builder, "da_tp");
	vp_tp_bodies = gtk_builder_get_object(builder, "vp_tp_bodies");

	tp_system_camera = new_camera(GTK_WIDGET(da_tp), &on_tp_screen_resize, &on_enable_camera_rotation, &on_disable_camera_rotation, &on_tp_screen_mouse_move, &on_tp_screen_scroll);


	tp_system = NULL;
	curr_transfer_tp = NULL;

	fix_tfbody = TRUE;

	create_combobox_dropdown_text_renderer(cb_tp_system, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_tp_central_body, GTK_ALIGN_CENTER);
	create_combobox_dropdown_text_renderer(cb_tp_tfbody, GTK_ALIGN_CENTER);
	if(get_num_available_systems() > 0) {
		update_system_dropdown(GTK_COMBO_BOX(cb_tp_system));
		tp_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_system))];
		update_central_body_dropdown(GTK_COMBO_BOX(cb_tp_central_body), tp_system);
		tp_update_bodies();

		update_camera_to_celestial_system(tp_system_camera, tp_system, deg2rad(90), 0);
	}

	update_date_label();
	update_transfer_panel();
}


// TRANSFER PLANNER SYSTEM VIEW CALLBACKS -----------------------------------------------
void update_tp_system_view() {
	clear_camera_screen(tp_system_camera);
	if(tp_system == NULL) return;

	draw_body(tp_system_camera, tp_system, tp_system->cb, current_date_tp);

	for(int i = 0; i < tp_system->num_bodies; i++) {
		if(!body_show_status_tp[i]) continue;
		draw_body(tp_system_camera, tp_system, tp_system->bodies[i], current_date_tp);
		struct Orbit orbit = tp_system->bodies[i]->orbit;
		if(tp_system->prop_method == EPHEMS) {
			struct OSV body_osv = osv_from_ephem(tp_system->bodies[i]->ephem, tp_system->bodies[i]->num_ephems, current_date_tp, tp_system->cb);
			orbit = constr_orbit_from_osv(body_osv.r, body_osv.v, tp_system->cb);
		}
		draw_orbit(tp_system_camera, orbit);
	}

	draw_itinerary(tp_system_camera, tp_system, curr_transfer_tp, current_date_tp);

	draw_camera_image(tp_system_camera);
}

void on_tp_screen_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer *ptr) {
	on_camera_zoom(widget, event, tp_system_camera);
	update_tp_system_view();
}

void on_tp_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr) {
	resize_camera_screen(tp_system_camera);
	update_tp_system_view();
}

void on_tp_screen_mouse_move(GtkWidget *widget, GdkEventButton *event, gpointer *ptr) {
	if (tp_system_camera->rotation_sensitive) {
		on_camera_rotate(tp_system_camera, event);
		update_tp_system_view();
	}
}

// -------------------------------------------------------------------------------------


G_MODULE_EXPORT void on_tp_system_change() {
	if(get_num_available_systems() == 0 ||
			gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_system)) == get_num_available_systems() ||
			tp_system == get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_system))] ||
			gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_system)) == -1) return;
	
	if(!is_available_system(get_top_level_system(tp_system)) && tp_system != NULL) {
		free_celestial_system(get_top_level_system(tp_system));
		tp_system = NULL;
		remove_combobox_last_entry(GTK_COMBO_BOX(cb_tp_system));
	}

	tp_system = get_available_systems()[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_system))];
	update_central_body_dropdown(GTK_COMBO_BOX(cb_tp_central_body), tp_system);
	update_camera_to_celestial_system(tp_system_camera, tp_system, deg2rad(90), 0);
	remove_all_transfers();
	tp_update_bodies();
	update();
}


G_MODULE_EXPORT void on_tp_central_body_change() {
	if(get_number_of_subsystems(get_top_level_system(tp_system)) == 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(cb_tp_central_body), 0);
		return;
	}
	gtk_widget_set_sensitive(GTK_WIDGET(cb_tp_central_body), 1);
	tp_system = get_subsystem_from_system_and_id(get_top_level_system(tp_system), gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_central_body)));

	update_camera_to_celestial_system(tp_system_camera, tp_system, deg2rad(90), 0);
	remove_all_transfers();
	tp_update_bodies();
	update();
}

double calc_step_dv(struct ItinStep *step) {
	if(step == NULL || (step->prev == NULL && step->next == NULL)) return 0;
	if(step->body == NULL) {
		if(step->next == NULL || step->next[0]->next == NULL || step->prev == NULL)
			return 0;
		if(step->v_body.x == 0) return 0;
		return mag_vec3(subtract_vec3(step->v_arr, step->next[0]->v_dep));
	} else if(step->prev == NULL) {
		double vinf = mag_vec3(subtract_vec3(step->next[0]->v_dep, step->v_body));
		return dv_circ(step->body, altatmo2radius(step->body, tp_dep_periapsis), vinf);
	} else if(step->next == NULL) {
		if(tp_last_transfer_type == TF_FLYBY) return 0;
		double vinf = mag_vec3(subtract_vec3(step->v_arr, step->v_body));
		if(tp_last_transfer_type == TF_CAPTURE) return dv_capture(step->body, altatmo2radius(step->body, tp_arr_periapsis), vinf);
		else if(tp_last_transfer_type == TF_CIRC) return dv_circ(step->body, altatmo2radius(step->body, tp_arr_periapsis), vinf);
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
	if(curr_transfer_tp != NULL) {
		tp_dep_periapsis = get_first(curr_transfer_tp)->body->atmo_alt + 50e3;
		tp_arr_periapsis = get_last(curr_transfer_tp)->body->atmo_alt + 50e3;
	}
	update_itin_body_osvs(get_first(curr_transfer_tp), tp_system);
	calc_itin_v_vectors_from_dates_and_r(get_first(curr_transfer_tp), tp_system);
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
	update_tp_system_view();
}

void update_date_label() {
	char time_string[20];
	char date_string[20];
	char clock_string[20];
	date_to_string(convert_JD_date(current_date_tp, get_settings_datetime_type()), date_string, 0);
	clocktime_to_string(convert_JD_date(current_date_tp, get_settings_datetime_type()), clock_string, 0);
	sprintf(time_string, "%s %s", date_string, clock_string);
	gtk_label_set_text(GTK_LABEL(lb_tp_date), time_string);
}

void update_transfer_panel() {
	if(curr_transfer_tp == NULL) {
		gtk_button_set_label(GTK_BUTTON(tb_tp_tfdate), get_settings_datetime_type() == DATE_ISO ? "0000-00-00" : "0000-000");
		update_body_dropdown(GTK_COMBO_BOX(cb_tp_tfbody), NULL);
		fix_tfbody = TRUE;
	} else {
		struct Datetime date = convert_JD_date(curr_transfer_tp->date, get_settings_datetime_type());
		char date_string[10];
		date_to_string(date, date_string, 0);
		gtk_button_set_label(GTK_BUTTON(tb_tp_tfdate), date_string);
		if(gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_tfbody)) < 0) update_body_dropdown(GTK_COMBO_BOX(cb_tp_tfbody), tp_system);
		gtk_combo_box_set_active(GTK_COMBO_BOX(cb_tp_tfbody), get_body_system_id(curr_transfer_tp->body, tp_system));
		fix_tfbody = FALSE;
		char s_dv[20];
		sprintf(s_dv, "%6.0f m/s", calc_step_dv(curr_transfer_tp));
		gtk_label_set_label(GTK_LABEL(lb_tp_transfer_dv), s_dv);
		sprintf(s_dv, "%6.0f m/s", calc_total_dv());
		gtk_label_set_label(GTK_LABEL(lb_tp_total_dv), s_dv);
		if(curr_transfer_tp->prev != NULL || curr_transfer_tp->num_next_nodes > 0) {
			char s_tfprop_labels[200], s_tfprop_values[100];
			itinerary_step_parameters_to_string(s_tfprop_labels, s_tfprop_values, get_settings_datetime_type(), tp_dep_periapsis, tp_arr_periapsis, curr_transfer_tp);
			gtk_label_set_label(GTK_LABEL(lb_tp_param_labels), s_tfprop_labels);
			gtk_label_set_label(GTK_LABEL(lb_tp_param_values), s_tfprop_values);
		} else {
			gtk_label_set_label(GTK_LABEL(lb_tp_param_labels), "");
			gtk_label_set_label(GTK_LABEL(lb_tp_param_values), "");
		}
	}

}

void tp_update_show_body_list() {
	// Remove grid if exists
	if (grid_tp_bodies != NULL && GTK_WIDGET(vp_tp_bodies) == gtk_widget_get_parent(grid_tp_bodies)) {
		gtk_container_remove(GTK_CONTAINER(vp_tp_bodies), grid_tp_bodies);
	}

	grid_tp_bodies = gtk_grid_new();

	// Create a GtkLabel
	GtkWidget *label = gtk_label_new("Show Orbit");
	// width request
	gtk_widget_set_size_request(GTK_WIDGET(label), -1, -1);
	// set css class
	set_css_class_for_widget(GTK_WIDGET(label), "pag-header");

	// Set the label in the grid at the specified row and column
	gtk_grid_attach(GTK_GRID(grid_tp_bodies), label, 0, 0, 1, 1);
	gtk_widget_set_halign(label, GTK_ALIGN_START);

	// Create labels and buttons and add them to the grid
	for (int body_idx = 0; body_idx < tp_system->num_bodies; body_idx++) {
		int row = body_idx+1;
		GtkWidget *widget;
		// Create a show body check button
		widget = gtk_check_button_new_with_label(tp_system->bodies[body_idx]->name);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), body_show_status_tp[body_idx]);
		g_signal_connect(widget, "clicked", G_CALLBACK(on_body_toggle), &(body_show_status_tp[body_idx]));
		gtk_widget_set_halign(widget, GTK_ALIGN_START);

		// Set the label in the grid at the specified row and column
		gtk_grid_attach(GTK_GRID(grid_tp_bodies), widget, 0, row, 1, 1);
	}
	gtk_container_add (GTK_CONTAINER (vp_tp_bodies), grid_tp_bodies);
	gtk_widget_show_all(GTK_WIDGET(vp_tp_bodies));
}

void show_bodies_of_itinerary(struct ItinStep *step) {
	while(step != NULL) {
		if(step->body != NULL) {
			body_show_status_tp[get_body_system_id(step->body, tp_system)] = 1;
		}
		if(step->num_next_nodes > 0) step = step->next[0];
		else step = NULL;
	}
}

void tp_update_bodies() {
	if(body_show_status_tp != NULL) free(body_show_status_tp);
	body_show_status_tp = (gboolean*) malloc(tp_system->num_bodies*sizeof(gboolean));
	if(curr_transfer_tp != NULL) {
		for(int i = 0; i < tp_system->num_bodies; i++) body_show_status_tp[i] = 0;
		show_bodies_of_itinerary(get_first(curr_transfer_tp));
	} else {
		for(int i = 0; i < tp_system->num_bodies; i++) body_show_status_tp[i] = 1;
	}
	tp_update_show_body_list();
}


G_MODULE_EXPORT void on_body_toggle(GtkWidget* widget, gpointer data) {
	gboolean *show_body = (gboolean *) data;  // Cast data back to group struct
	*show_body = !*show_body;
	update_tp_system_view();
}



void tp_change_date_type(enum DateType old_date_type, enum DateType new_date_type) {
	change_label_date_type(lb_tp_date, old_date_type, new_date_type);
	if(curr_transfer_tp != NULL) change_button_date_type(tb_tp_tfdate, old_date_type, new_date_type);
	current_date_tp = convert_date_JD(change_date_type(convert_JD_date(current_date_tp, old_date_type), new_date_type));
	
	if(strcmp(gtk_widget_get_name(GTK_WIDGET(bt_tp_10y6hp)), "+10Y") == 0) {
		gtk_button_set_label(GTK_BUTTON(bt_tp_1m30d10minp), new_date_type == DATE_ISO ? "+1M" : "+30D");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1m30d10minp), new_date_type == DATE_ISO ? "+1M" : "+30D");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1m30d10minm), new_date_type == DATE_ISO ? "-1M" : "-30D");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1m30d10minm), new_date_type == DATE_ISO ? "-1M" : "-30D");
	}
	update();
}

G_MODULE_EXPORT void on_change_date(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	// Date
	if		(strcmp(name, "+10Y") == 0) current_date_tp = jd_change_date(current_date_tp, 10, 0, 0, get_settings_datetime_type());
	else if	(strcmp(name,  "+1Y") == 0) current_date_tp = jd_change_date(current_date_tp, 1, 0, 0, get_settings_datetime_type());
	else if	(strcmp(name,  "+1M") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 1, 0, get_settings_datetime_type());
	else if	(strcmp(name,  "+30D") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0, 30, get_settings_datetime_type());
	else if	(strcmp(name,  "+1D") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0, 1, get_settings_datetime_type());
	else if	(strcmp(name, "-10Y") == 0) current_date_tp = jd_change_date(current_date_tp, -10, 0, 0, get_settings_datetime_type());
	else if	(strcmp(name,  "-1Y") == 0) current_date_tp = jd_change_date(current_date_tp, -1, 0, 0, get_settings_datetime_type());
	else if	(strcmp(name,  "-1M") == 0) current_date_tp = jd_change_date(current_date_tp, 0, -1, 0, get_settings_datetime_type());
	else if	(strcmp(name, "-30D") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0, -30, get_settings_datetime_type());
	else if	(strcmp(name,  "-1D") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0, -1, get_settings_datetime_type());
	// Clocktime
	else if	(strcmp(name,  "+6h") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0,  6.0/24, get_settings_datetime_type());
	else if	(strcmp(name,  "-6h") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0, -6.0/24, get_settings_datetime_type());
	else if	(strcmp(name,  "+1h") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0,  1.0/24, get_settings_datetime_type());
	else if	(strcmp(name,  "-1h") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0, -1.0/24, get_settings_datetime_type());
	else if	(strcmp(name, "+10m") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0,  10.0/(24*60), get_settings_datetime_type());
	else if	(strcmp(name, "-10m") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0, -10.0/(24*60), get_settings_datetime_type());
	else if	(strcmp(name,  "+1m") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0,  1.0/(24*60), get_settings_datetime_type());
	else if	(strcmp(name,  "-1m") == 0) current_date_tp = jd_change_date(current_date_tp, 0, 0, -1.0/(24*60), get_settings_datetime_type());

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

G_MODULE_EXPORT void on_tp_reset_clocktime(GtkWidget* widget, gpointer data) {
	Datetime datetime = convert_JD_date(current_date_tp, get_settings_datetime_type());
	current_date_tp = convert_date_JD((Datetime){datetime.y, datetime.m, datetime.d});
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

G_MODULE_EXPORT void on_tp_switch_clocktime_date(GtkWidget* widget, gpointer data) {
	if(strcmp(gtk_widget_get_name(GTK_WIDGET(bt_tp_10y6hp)), "+10Y") == 0) {
		gtk_button_set_label(GTK_BUTTON(bt_tp_10y6hp), "+6h");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_10y6hp), "+6h");
		gtk_button_set_label(GTK_BUTTON(bt_tp_10y6hm), "-6h");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_10y6hm), "-6h");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1y1hp), "+1h");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1y1hp), "+1h");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1y1hm), "-1h");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1y1hm), "-1h");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1m30d10minp), "+10m");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1m30d10minp), "+10m");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1m30d10minm), "-10m");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1m30d10minm), "-10m");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1d1minp), "+1m");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1d1minp), "+1m");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1d1minm), "-1m");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1d1minm), "-1m");
	} else {
		gtk_button_set_label(GTK_BUTTON(bt_tp_10y6hp), "+10Y");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_10y6hp), "+10Y");
		gtk_button_set_label(GTK_BUTTON(bt_tp_10y6hm), "-10Y");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_10y6hm), "-10Y");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1y1hp), "+1Y");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1y1hp), "+1Y");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1y1hm), "-1Y");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1y1hm), "-1Y");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1m30d10minp), get_settings_datetime_type() == DATE_ISO ? "+1M" : "+30D");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1m30d10minp), get_settings_datetime_type() == DATE_ISO ? "+1M" : "+30D");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1m30d10minm), get_settings_datetime_type() == DATE_ISO ? "-1M" : "-30D");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1m30d10minm), get_settings_datetime_type() == DATE_ISO ? "-1M" : "-30D");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1d1minp), "+1D");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1d1minp), "+1D");
		gtk_button_set_label(GTK_BUTTON(bt_tp_1d1minm), "-1D");
		gtk_widget_set_name(GTK_WIDGET(bt_tp_1d1minm), "-1D");
		
	}
}

G_MODULE_EXPORT void on_prev_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	if(curr_transfer_tp->prev != NULL) curr_transfer_tp = curr_transfer_tp->prev;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	current_date_tp = curr_transfer_tp->date;
	update();
}

G_MODULE_EXPORT void on_next_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	if(curr_transfer_tp->next != NULL) curr_transfer_tp = curr_transfer_tp->next[0];
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	current_date_tp = curr_transfer_tp->date;
	update();
}

G_MODULE_EXPORT void on_transfer_body_change(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL || fix_tfbody) return;
	curr_transfer_tp->body = tp_system->bodies[gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_tfbody))];
	update_itinerary();
}

G_MODULE_EXPORT void on_last_transfer_type_changed_tp(GtkWidget* widget, gpointer data) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) return;
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "fb") == 0) tp_last_transfer_type = TF_FLYBY;
	else if	(strcmp(name, "capture") == 0) tp_last_transfer_type = TF_CAPTURE;
	else if	(strcmp(name, "circ") == 0) tp_last_transfer_type = TF_CIRC;
	update_transfer_panel();
}

G_MODULE_EXPORT void on_toggle_transfer_date_lock(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
		return;
	}
	if(curr_transfer_tp->body == NULL)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	if(curr_transfer_tp != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate)))
		current_date_tp = curr_transfer_tp->date;
	update_itinerary();
}

G_MODULE_EXPORT void on_goto_transfer_date(GtkWidget* widget, gpointer data) {
	if(curr_transfer_tp == NULL) return;
	current_date_tp = curr_transfer_tp->date;
	update_itinerary();
}

G_MODULE_EXPORT void on_add_transfer(GtkWidget* widget, gpointer data) {
	if(tp_system == NULL) return;
	struct ItinStep *new_transfer = (struct ItinStep *) malloc(sizeof(struct ItinStep));
	new_transfer->body = tp_system->home_body ? : tp_system->bodies[0];
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

G_MODULE_EXPORT void on_remove_transfer(GtkWidget* widget, gpointer data) {
	if(tp_system == NULL) return;
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
	find_viable_flybys(temp, tp_system, step->body, 86400, 86400*365.25*50);

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

G_MODULE_EXPORT void on_find_closest_transfer(GtkWidget* widget, gpointer data) {
	if(tp_system == NULL) return;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	int success = find_closest_transfer(curr_transfer_tp);
	if(success) update_itinerary();
}

G_MODULE_EXPORT void on_show_itin_overview() {
	if(curr_transfer_tp == NULL) return;
	char text[10000];
	itinerary_short_overview_to_string(curr_transfer_tp, get_settings_datetime_type(), tp_dep_periapsis, tp_arr_periapsis, text);
//	itinerary_detailed_overview_to_string(curr_transfer_tp, get_settings_datetime_type(), tp_dep_periapsis, tp_arr_periapsis, text);
	show_msg_window(text);
}

//G_MODULE_EXPORT void on_find_itinerary(GtkWidget* widget, gpointer data) {
//	if(tp_system == NULL || curr_transfer_tp == NULL) return;
//	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
//	struct ItinStep *itin_copy = create_itin_copy(get_first(curr_transfer_tp));
//	while(itin_copy->prev != NULL) {
//		if(itin_copy->prev->body == NULL) return;	// double swing-by not implemented
//		itin_copy = itin_copy->prev;
//	}
//	if(itin_copy == NULL || itin_copy->next == NULL || itin_copy->next[0]->next == NULL) return;
//
//	itin_copy = itin_copy->next[0];
//	int status = 1;
//
//	while(itin_copy->next != NULL) {
//		itin_copy = itin_copy->next[0];
//		status = find_closest_transfer(itin_copy);
//		if(!status) break;
//	}
//
//	if(status) {
//		while(itin_copy->prev != NULL) itin_copy = itin_copy->prev;
//		struct ItinStep *itin = get_first(curr_transfer_tp);
//		while(itin != NULL) {
//			copy_step_body_vectors_and_date(itin_copy, itin);
//			if(itin->next != NULL) {
//				itin = itin->next[0];
//				itin_copy = itin_copy->next[0];
//			} else {
//				itin = NULL;
//			}
//		}
//		update_itinerary();
//	}
//
//	free_itinerary(itin_copy);
//}

G_MODULE_EXPORT void on_save_itinerary(GtkWidget* widget, gpointer data) {
	if(tp_system == NULL) return;
	struct ItinStep *first = get_first(curr_transfer_tp);
	if(first == NULL || !is_valid_itinerary(get_last(curr_transfer_tp))) return;

	char filepath[255];
	if(!get_path_from_file_chooser(filepath, ".itin", GTK_FILE_CHOOSER_ACTION_SAVE, "")) return;
	store_single_itinerary_in_bfile(first, tp_system, filepath);
}

G_MODULE_EXPORT void on_load_itinerary(GtkWidget* widget, gpointer data) {
	char filepath[255];
	if(!get_path_from_file_chooser(filepath, ".itin", GTK_FILE_CHOOSER_ACTION_OPEN, "")) return;

	if(curr_transfer_tp != NULL) {
		fix_tfbody = TRUE;
		update_body_dropdown(GTK_COMBO_BOX(cb_tp_tfbody), NULL);
		free_itinerary(get_first(curr_transfer_tp));
	}
	if(!is_available_system(tp_system) && tp_system != NULL) free_celestial_system(tp_system);
	curr_transfer_tp = NULL;
	tp_system = NULL;
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(cb_tp_system)) == get_num_available_systems()) remove_combobox_last_entry(GTK_COMBO_BOX(cb_tp_system));

	struct ItinLoadFileResults load_results = load_single_itinerary_from_bfile(filepath);
	curr_transfer_tp = get_first(load_results.itin);
	tp_system = load_results.system;
	current_date_tp = curr_transfer_tp->date;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tp_tfdate), 0);
	tp_update_bodies();
	update_itinerary();
	update_camera_to_celestial_system(tp_system_camera, tp_system, deg2rad(90), 0);
	camera_zoom_to_fit_itinerary(tp_system_camera, curr_transfer_tp);
	update_tp_system_view();
	char system_name[50];
	sprintf(system_name, "- %s -", tp_system->name);
	append_combobox_entry(GTK_COMBO_BOX(cb_tp_system), system_name);
	update_central_body_dropdown(GTK_COMBO_BOX(cb_tp_central_body), tp_system);

	struct ItinStep *step2pr = get_first(curr_transfer_tp);
	HyperbolaParams hyp_params = get_hyperbola_params(vec3(0,0,0), step2pr->next[0]->v_dep, step2pr->v_body,
																		   step2pr->body, 200e3, HYP_DEPARTURE);
	printf("\nDeparture Hyperbola %s\n"
		   "Date: %f\n"
		   "OutgoingRadPer: %f km\n"
		   "OutgoingC3Energy: %f km²/s²\n"
		   "OutgoingRHA: %f°\n"
		   "OutgoingDHA: %f°\n"
		   "OutgoingBVAZI: -°\n"
		   "TA: 0.0°\n",
		   step2pr->body->name, step2pr->date, hyp_params.rp/1000, hyp_params.c3_energy/1e6,
		   rad2deg(hyp_params.outgoing.bplane_angle), rad2deg(hyp_params.outgoing.decl));
	
	step2pr = step2pr->next[0];
	while(step2pr->num_next_nodes != 0) {
		if(step2pr->body != NULL) {
			Vector3 v_arr = step2pr->v_arr;
			Vector3 v_dep = step2pr->next[0]->v_dep;
			Vector3 v_body = step2pr->v_body;
			double incl = get_flyby_inclination(v_arr, v_dep, v_body, get_body_equatorial_plane(step2pr->body));

			hyp_params = get_hyperbola_params(step2pr->v_arr, step2pr->next[0]->v_dep, step2pr->v_body, step2pr->body, 0, HYP_FLYBY);
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
				   step2pr->body->name, dt_in_days, step2pr->date, hyp_params.rp / 1000, rad2deg(incl),
				   hyp_params.c3_energy / 1e6,
				   rad2deg(hyp_params.incoming.bplane_angle), rad2deg(hyp_params.incoming.decl),
				   rad2deg(hyp_params.incoming.bvazi),
				   rad2deg(hyp_params.outgoing.bplane_angle), rad2deg(hyp_params.outgoing.decl),
				   rad2deg(hyp_params.outgoing.bvazi));
			step2pr = step2pr->next[0];
		} else {
			double dt_in_days = step2pr->date - step2pr->prev->date;
			double dist_to_sun = mag_vec3(step2pr->r);

			Vector3 orbit_prograde = step2pr->v_arr;
			Vector3 orbit_normal = cross_vec3(step2pr->r, step2pr->v_arr);
			Vector3 orbit_radialin = cross_vec3(orbit_normal, step2pr->v_arr);
			Vector3 dv_vec = subtract_vec3(step2pr->next[0]->v_dep, step2pr->v_arr);

			// dv vector in S/C coordinate system (prograde, radial in, normal) (sign it if projected vector more than 90° from target vector / pointing in opposite direction)
			Vector3 dv_vec_sc = {
					mag_vec3(proj_vec3_vec3(dv_vec, orbit_prograde)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_prograde), orbit_prograde) < M_PI/2 ? 1 : -1),
					mag_vec3(proj_vec3_vec3(dv_vec, orbit_radialin)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_radialin), orbit_radialin) < M_PI/2 ? 1 : -1),
					mag_vec3(proj_vec3_vec3(dv_vec, orbit_normal)) * (angle_vec3_vec3(proj_vec3_vec3(dv_vec, orbit_normal), orbit_normal) < M_PI/2 ? 1 : -1)
			};

			printf("\nDeep Space Maneuver (Travel Time: %.2f days)\n"
				   "Date: %f\n"
				   "Distance to the Sun: %.3f AU\n"
				   "Dv Prograde: %f m/s\n"
				   "Dv Radial: %f m/s\n"
				   "Dv Normal: %f m/s\n"
				   "Total: %f m/s\n",
				   dt_in_days, step2pr->date, dist_to_sun / 1.495978707e11, dv_vec_sc.x, dv_vec_sc.y, dv_vec_sc.z, mag_vec3(dv_vec_sc));
			step2pr = step2pr->next[0];
		}
	}
	
	double dt_in_days = step2pr->date - step2pr->prev->date;
	hyp_params = get_hyperbola_params(step2pr->v_arr, vec3(0,0,0), step2pr->v_body, step2pr->body, 200e3, HYP_ARRIVAL);
	printf("\nArrival Hyperbola %s (Travel Time: %.2f days)\n"
		   "Date: %f\n"
		   "IncomingRadPer: %f km\n"
		   "IncomingC3Energy: %f km²/s²\n"
		   "IncomingRHA: %f°\n"
		   "IncomingDHA: %f°\n"
		   "IncomingBVAZI: -°\n"
		   "TA: 0.0°\n",
		   step2pr->body->name, dt_in_days, step2pr->date, hyp_params.rp/1000, hyp_params.c3_energy/1e6,
		   rad2deg(hyp_params.incoming.bplane_angle), rad2deg(hyp_params.incoming.decl));
}


G_MODULE_EXPORT void on_create_gmat_script() {
	if(curr_transfer_tp != NULL) write_gmat_script(curr_transfer_tp, "transfer.script");
}

void end_transfer_planner() {
	remove_all_transfers();
	if(body_show_status_tp != NULL) free(body_show_status_tp);
	destroy_camera(tp_system_camera);
}