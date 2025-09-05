#include "porkchop_analyzer.h"
#include "porkchop_analyzer_tools.h"
#include "gui/drawing.h"
#include "gui/gui_manager.h"
#include "gui/css_loader.h"
#include "gui/settings.h"
#include "tools/file_io.h"
#include "gui/info_win_manager.h"

#include <string.h>
#include <locale.h>
#include <math.h>

int pa_num_deps, pa_num_itins;
struct ItinStep **pa_departures;
enum LastTransferType pa_last_transfer_type;
GObject *da_pa_porkchop, *da_pa_preview;
struct ItinStep *curr_transfer_pa;
double current_date_pa;
int pa_num_groups = 0;
struct PorkchopGroup *pa_groups;
struct PorkchopAnalyzerPoint *pa_porkchop_points;

ItinStepBinHeaderData pa_analysis_params;

CelestSystem *pa_system;

double pa_min_vals[6], pa_max_vals[6];

enum PaMinMaxType {PA_DEP, PA_ARR, PA_DUR, PA_TOTDV, PA_DEPDV, PA_SATDV};
enum PaYAxisType {PA_YAXIS_DUR, PA_YAXIS_ARRDATE};

enum PaYAxisType pa_yaxis_type = PA_YAXIS_DUR;
double pa_dep_periapsis = 50e3;
double pa_arr_periapsis = 50e3;

Screen *pa_porkchop_screen;
Camera *pa_itin_preview_camera;

gboolean *body_show_status_pa;
GObject *tf_pa_min_feedback[6];
GObject *tf_pa_max_feedback[6];
GObject *lb_pa_tfdate;
GObject *lb_pa_tfbody;
GObject *lb_pa_transfer_dv;
GObject *lb_pa_total_dv;
GObject *lb_pa_periapsis;
GObject *st_pa_step_group_selector;
GObject *vp_pa_groups;
GtkWidget *grid_pa_groups;

void update_pa();
void on_pa_screen_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer *ptr);
void on_pa_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr);
void on_pa_screen_mouse_move(GtkWidget *widget, GdkEventButton *event, gpointer *ptr);
void on_pa_screen_button_release(GtkWidget *widget, GdkEventButton *event, gpointer *ptr);


void init_porkchop_analyzer(GtkBuilder *builder) {
	pa_analysis_params.num_deps = 0;
	pa_num_deps = 0;
	pa_num_itins = 0;
	pa_departures = NULL;
	pa_porkchop_points = NULL;
	curr_transfer_pa = NULL;
	pa_last_transfer_type = TF_FLYBY;
	da_pa_porkchop = gtk_builder_get_object(builder, "da_pa_porkchop");
	da_pa_preview = gtk_builder_get_object(builder, "da_pa_preview");
	tf_pa_min_feedback[PA_DEP] = gtk_builder_get_object(builder, "tf_pa_min_depdate");
	tf_pa_min_feedback[PA_ARR] = gtk_builder_get_object(builder, "tf_pa_min_arrdate");
	tf_pa_min_feedback[PA_DUR] = gtk_builder_get_object(builder, "tf_pa_min_dur");
	tf_pa_min_feedback[PA_TOTDV] = gtk_builder_get_object(builder, "tf_pa_min_totdv");
	tf_pa_min_feedback[PA_DEPDV] = gtk_builder_get_object(builder, "tf_pa_min_depdv");
	tf_pa_min_feedback[PA_SATDV] = gtk_builder_get_object(builder, "tf_pa_min_satdv");
	tf_pa_max_feedback[PA_DEP] = gtk_builder_get_object(builder, "tf_pa_max_depdate");
	tf_pa_max_feedback[PA_ARR] = gtk_builder_get_object(builder, "tf_pa_max_arrdate");
	tf_pa_max_feedback[PA_DUR] = gtk_builder_get_object(builder, "tf_pa_max_dur");
	tf_pa_max_feedback[PA_TOTDV] = gtk_builder_get_object(builder, "tf_pa_max_totdv");
	tf_pa_max_feedback[PA_DEPDV] = gtk_builder_get_object(builder, "tf_pa_max_depdv");
	tf_pa_max_feedback[PA_SATDV] = gtk_builder_get_object(builder, "tf_pa_max_satdv");
	lb_pa_tfdate = gtk_builder_get_object(builder, "lb_pa_tfdate");
	lb_pa_tfbody = gtk_builder_get_object(builder, "lb_pa_tfbody");
	lb_pa_transfer_dv = gtk_builder_get_object(builder, "lb_pa_transfer_dv");
	lb_pa_total_dv = gtk_builder_get_object(builder, "lb_pa_total_dv");
	lb_pa_periapsis = gtk_builder_get_object(builder, "lb_pa_periapsis");
	st_pa_step_group_selector = gtk_builder_get_object(builder, "st_pa_step_group_selector");
	vp_pa_groups = gtk_builder_get_object(builder, "vp_pa_groups");


	pa_porkchop_screen = new_screen(GTK_WIDGET(da_pa_porkchop), &on_pa_screen_resize, &on_screen_button_press, &on_pa_screen_button_release, &on_pa_screen_mouse_move, NULL);
	set_screen_background_color(pa_porkchop_screen, 0.15, 0.15, 0.15);
	pa_itin_preview_camera = new_camera(GTK_WIDGET(da_pa_preview), &on_pa_screen_resize, &on_enable_camera_rotation, &on_disable_camera_rotation, &on_pa_screen_mouse_move, &on_pa_screen_scroll);

	pa_system = NULL;
}


// ITINERARY PREVIEW AND PORKCHOP CALLBACKS -----------------------------------------------
void update_pa_porkchop_diagram() {
	clear_screen(pa_porkchop_screen);

	if(pa_porkchop_points != NULL) draw_porkchop(pa_porkchop_screen->static_layer.cr, pa_porkchop_screen->width, pa_porkchop_screen->height, pa_porkchop_points, pa_num_itins, pa_last_transfer_type, pa_yaxis_type);
	draw_screen(pa_porkchop_screen);
}

void update_pa_itinerary_preview() {
	clear_camera_screen(pa_itin_preview_camera);

	if(pa_system == NULL) return;

	// Sun
	draw_body(pa_itin_preview_camera, pa_system, pa_system->cb, current_date_pa);
	// Planets
	for(int i = 0; i < pa_system->num_bodies; i++) {
		if(!body_show_status_pa[i]) continue;
		draw_body(pa_itin_preview_camera, pa_system, pa_system->bodies[i], current_date_pa);
		struct Orbit orbit = pa_system->bodies[i]->orbit;
		if(pa_system->prop_method == EPHEMS) {
			struct OSV body_osv = osv_from_ephem(pa_system->bodies[i]->ephem, pa_system->bodies[i]->num_ephems, current_date_pa, pa_system->cb);
			orbit = constr_orbit_from_osv(body_osv.r, body_osv.v, pa_system->cb);
		}
		draw_orbit(pa_itin_preview_camera, orbit);
	}
	// Transfers
	if(curr_transfer_pa != NULL) draw_itinerary(pa_itin_preview_camera, pa_system, curr_transfer_pa, current_date_pa);

	draw_camera_image(pa_itin_preview_camera);
}

void on_pa_screen_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer *ptr) {
	if((Camera*)ptr == pa_itin_preview_camera) {
		on_camera_zoom(widget, event, pa_itin_preview_camera);
		update_pa_itinerary_preview();
	}
}

void on_pa_screen_resize(GtkWidget *widget, cairo_t *cr, gpointer *ptr) {
	if((Screen*)ptr == pa_porkchop_screen) {
		resize_screen(pa_porkchop_screen);
		update_pa_porkchop_diagram();
	}
	if((Camera*)ptr == pa_itin_preview_camera) {
		resize_camera_screen(pa_itin_preview_camera);
		update_pa_itinerary_preview();
	}
}

void on_pa_screen_mouse_move(GtkWidget *widget, GdkEventButton *event, gpointer *ptr) {
	if((Screen*)ptr == pa_porkchop_screen) {
		if(pa_system == NULL) return;
		int min_x = pa_yaxis_type == PA_YAXIS_ARRDATE ? get_porkchop_arrdate_yaxis_x() : get_porkchop_dur_yaxis_x();
		int min_y = get_porkchop_xaxis_y();
		if(pa_porkchop_screen->dragging) {
			double x = pa_porkchop_screen->mouse_pos_on_press.x;
			double y = pa_porkchop_screen->mouse_pos_on_press.y;
			double width = event->x-pa_porkchop_screen->mouse_pos_on_press.x;
			double height = event->y-pa_porkchop_screen->mouse_pos_on_press.y;
			if(x < min_x || y > pa_porkchop_screen->height-min_y) return;
			if(x+width < min_x) width = min_x-x;
			if(y+height < 0) height = -y;
			if(x+width > pa_porkchop_screen->width) width = pa_porkchop_screen->width-x;
			if(y+height > pa_porkchop_screen->height-min_y) height = (pa_porkchop_screen->height-min_y)-y;
			clear_dynamic_screen_layer(pa_porkchop_screen);
			cairo_rectangle(pa_porkchop_screen->dynamic_layer.cr, x, y, width, height);
			cairo_set_source_rgba(pa_porkchop_screen->dynamic_layer.cr, 1, 0, 0, 1);
			cairo_stroke(pa_porkchop_screen->dynamic_layer.cr);
			draw_screen(pa_porkchop_screen);
		}
	}
	if((Camera*)ptr == pa_itin_preview_camera && pa_itin_preview_camera->rotation_sensitive) {
		on_camera_rotate(pa_itin_preview_camera, event);
		resize_camera_screen(pa_itin_preview_camera);
		update_pa_itinerary_preview();
	}
}

void on_pa_screen_button_release(GtkWidget *widget, GdkEventButton *event, gpointer *ptr) {
	if((Screen*)ptr == pa_porkchop_screen) {
		pa_porkchop_screen->dragging = FALSE;

		clear_dynamic_screen_layer(pa_porkchop_screen);

		if(pa_system == NULL) return;

		double x0 = pa_porkchop_screen->mouse_pos_on_press.x;
		double y0 = pa_porkchop_screen->mouse_pos_on_press.y;
		double x1 = event->x;
		double y1 = event->y;

		if(x0 < 45 || y0 > pa_porkchop_screen->height-40) return;
		get_min_max_dep_arr_dur_range_from_mouse_rect(&x0, &x1, &y0, &y1, pa_min_vals[PA_DEP], pa_max_vals[PA_DEP],
													  pa_min_vals[pa_yaxis_type == PA_YAXIS_DUR ? PA_DUR : PA_ARR],
													  pa_max_vals[pa_yaxis_type == PA_YAXIS_DUR ? PA_DUR : PA_ARR],
													  pa_porkchop_screen->width, pa_porkchop_screen->height,
													  pa_yaxis_type);

		char string[20];
		date_to_string(convert_JD_date(x0, get_settings_datetime_type()), string, 0);
		gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[PA_DEP]), string);
		date_to_string(convert_JD_date(x1, get_settings_datetime_type()), string, 0);
		gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[PA_DEP]), string);
		
		if(pa_yaxis_type == PA_YAXIS_ARRDATE) {
			date_to_string(convert_JD_date(y0, get_settings_datetime_type()), string, 0);
			gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[PA_ARR]), string);
			date_to_string(convert_JD_date(y1, get_settings_datetime_type()), string, 0);
			gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[PA_ARR]), string);
		} else {
			sprintf(string, "%.0f", get_settings_datetime_type() == DATE_KERBAL ? y0*4 : y0);
			gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[PA_DUR]), string);
			sprintf(string, "%.0f", get_settings_datetime_type() == DATE_KERBAL ? y1*4 : y1);
			gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[PA_DUR]), string);
		}

		on_apply_filter(NULL, NULL);

		draw_screen(pa_porkchop_screen);
	}
}

// -------------------------------------------------------------------------------------


void pa_change_date_type(enum DateType old_date_type, enum DateType new_date_type) {
	change_text_field_date_type(tf_pa_min_feedback[PA_DEP], old_date_type, new_date_type);
	change_text_field_date_type(tf_pa_max_feedback[PA_DEP], old_date_type, new_date_type);
	change_text_field_date_type(tf_pa_min_feedback[PA_ARR], old_date_type, new_date_type);
	change_text_field_date_type(tf_pa_max_feedback[PA_ARR], old_date_type, new_date_type);
	if(curr_transfer_pa != NULL) change_label_date_type(lb_pa_tfdate, old_date_type, new_date_type);
	else if(new_date_type == DATE_ISO) gtk_label_set_text(GTK_LABEL(lb_pa_tfdate), "0000-00-00");
	else gtk_label_set_text(GTK_LABEL(lb_pa_tfdate), "0000-000");
	update_pa();
}

double calc_step_dv_pa(struct ItinStep *step) {
	if(step == NULL || (step->prev == NULL && step->next == NULL)) return 0;
	if(step->body == NULL) {
		if(step->next == NULL || step->next[0]->next == NULL || step->prev == NULL)
			return 0;
		return mag_vec3(subtract_vec3(step->v_arr, step->next[0]->v_dep));
	} else if(step->prev == NULL) {
		double vinf = mag_vec3(subtract_vec3(step->next[0]->v_dep, step->v_body));
		return dv_circ(step->body, altatmo2radius(step->body, pa_dep_periapsis), vinf);
	} else if(step->next == NULL) {
		if(pa_last_transfer_type == TF_FLYBY) return 0;
		double vinf = mag_vec3(subtract_vec3(step->v_arr, step->v_body));
		if(pa_last_transfer_type == TF_CAPTURE) return dv_capture(step->body, altatmo2radius(step->body, pa_arr_periapsis), vinf);
		else if(pa_last_transfer_type == TF_CIRC) return dv_circ(step->body, altatmo2radius(step->body, pa_arr_periapsis), vinf);
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
	double rp = get_flyby_periapsis(curr_transfer_pa->v_arr, curr_transfer_pa->next[0]->v_dep, curr_transfer_pa->v_body, curr_transfer_pa->body);
	return (rp-curr_transfer_pa->body->radius)*1e-3;
}

void free_all_porkchop_analyzer_itins() {
	if(pa_departures != NULL) {
		for(int i = 0; i < pa_num_deps; i++) free(pa_departures[i]);
		free(pa_departures);
		pa_departures = NULL;
	}
	if(pa_porkchop_points != NULL) free(pa_porkchop_points);
	pa_porkchop_points = NULL;
	if(curr_transfer_pa != NULL) free_itinerary(get_first(curr_transfer_pa));
	curr_transfer_pa = NULL;
	free(pa_groups);
	pa_groups = NULL;
	if(!is_available_system(pa_system)) free_celestial_system(pa_system);
	pa_system = NULL;
	
	if(pa_analysis_params.num_deps > 0 && pa_analysis_params.file_type > 2) {
		if(pa_analysis_params.calc_data.seq_info.to_target.type == ITIN_SEQ_INFO_TO_TARGET) free(pa_analysis_params.calc_data.seq_info.to_target.flyby_bodies);
		if(pa_analysis_params.calc_data.seq_info.spec_seq.type == ITIN_SEQ_INFO_SPEC_SEQ) free(pa_analysis_params.calc_data.seq_info.spec_seq.bodies);
	}
	pa_analysis_params.num_deps = 0;
}

void pa_update_body_show_status() {
	if(pa_system == NULL) return;
	struct ItinStep *step = get_last(curr_transfer_pa);
	for(int i = 0; i < pa_system->num_bodies; i++) body_show_status_pa[i] = 0;
	if(step == NULL) return;
	while(step != NULL) {
		if(step->body != NULL) body_show_status_pa[get_body_system_id(step->body,pa_system)] = 1;
		step = step->prev;
	}
}

void pa_update_preview() {
	pa_update_body_show_status();
	update_pa_itinerary_preview();

	if(curr_transfer_pa == NULL) {
		char date0_string[10];
		date_to_string((Datetime) {.date_type=get_settings_datetime_type()}, date0_string, 0);
		gtk_label_set_label(GTK_LABEL(lb_pa_tfdate), date0_string);
		gtk_label_set_label(GTK_LABEL(lb_pa_tfbody), "Planet");
	} else {
		Datetime date = convert_JD_date(curr_transfer_pa->date, get_settings_datetime_type());
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

void reset_min_max_feedback(int take_hidden_group_into_account) {
	if(pa_num_itins == 0) return;
	int cap = pa_last_transfer_type == TF_CAPTURE, circ = pa_last_transfer_type == TF_CIRC;

	int first_show_idx = 0;
	while(!pa_porkchop_points[first_show_idx].inside_filter || (!take_hidden_group_into_account && !pa_porkchop_points[first_show_idx].group->show_group)) first_show_idx++;
	struct PorkchopPoint pp = pa_porkchop_points[first_show_idx].data;

	double min[6] = {
			/* depdate	*/ pp.dep_date,
			/* arrdate	*/ pp.dep_date+pp.dur,
			/* duration	*/ pp.dur,
			/* total dv	*/ pp.dv_dep + pp.dv_dsm + pp.dv_arr_cap*cap + pp.dv_arr_circ*circ,
			/* dep dv	*/ pp.dv_dep,
			/* sat dv	*/ pp.dv_dsm + pp.dv_arr_cap*cap + pp.dv_arr_circ*circ,
	};
	double max[6] = {
			/* depdate	*/ pp.dep_date,
			/* arrdate	*/ pp.dep_date+pp.dur,
			/* duration	*/ pp.dur,
			/* total dv	*/ pp.dv_dep + pp.dv_dsm + pp.dv_arr_cap*cap + pp.dv_arr_circ*circ,
			/* dep dv	*/ pp.dv_dep,
			/* sat dv	*/ pp.dv_dsm + pp.dv_arr_cap*cap + pp.dv_arr_circ*circ,
	};
	double dep_dv, sat_dv, tot_dv, depdate, arrdate, dur;

	for(int i = 1; i < pa_num_itins; i++) {
		if(!pa_porkchop_points[i].inside_filter || (!take_hidden_group_into_account && !pa_porkchop_points[i].group->show_group)) continue;
		pp = pa_porkchop_points[i].data;
		dep_dv = pp.dv_dep;
		sat_dv = pp.dv_dsm + pp.dv_arr_cap*cap + pp.dv_arr_circ*circ;
		tot_dv = dep_dv + sat_dv;
		depdate = pp.dep_date;
		dur = pp.dur;
		arrdate = depdate + dur;
		
		int j = 0;
		
		if(depdate < min[j]) min[j] = depdate;
		else if(depdate > max[j]) max[j] = depdate;
		j++;
		if(arrdate < min[j]) min[j] = arrdate;
		else if(arrdate > max[j]) max[j] = arrdate;
		j++;
		if(dur < min[j]) min[j] = dur;
		else if(dur > max[j]) max[j] = dur;
		j++;
		if(tot_dv < min[j]) min[j] = tot_dv;
		else if(tot_dv > max[j]) max[j] = tot_dv;
		j++;
		if(dep_dv < min[j]) min[j] = dep_dv;
		else if(dep_dv > max[j]) max[j] = dep_dv;
		j++;
		if(sat_dv < min[j]) min[j] = sat_dv;
		else if(sat_dv > max[j]) max[j] = sat_dv;
	}

	char string[20];
	date_to_string(convert_JD_date(min[PA_DEP], get_settings_datetime_type()), string, 0);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[PA_DEP]), string);
	date_to_string(convert_JD_date(min[PA_ARR], get_settings_datetime_type()), string, 0);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[PA_ARR]), string);
	sprintf(string, "%.0f", get_settings_datetime_type() == DATE_KERBAL ? min[PA_DUR]*4 : min[PA_DUR]);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[PA_DUR]), string);

	for(int i = 3; i < 6; i++) {
		sprintf(string, "%.2f", min[i]);
		gtk_entry_set_text(GTK_ENTRY(tf_pa_min_feedback[i]), string);
	}

	date_to_string(convert_JD_date(max[PA_DEP], get_settings_datetime_type()), string, 0);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[PA_DEP]), string);
	date_to_string(convert_JD_date(max[PA_ARR], get_settings_datetime_type()), string, 0);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[PA_ARR]), string);
	sprintf(string, "%.0f", get_settings_datetime_type() == DATE_KERBAL ? max[PA_DUR]*4 : max[PA_DUR]);
	gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[PA_DUR]), string);

	for(int i = 3; i < 6; i++) {
		sprintf(string, "%.2f", max[i]);
		gtk_entry_set_text(GTK_ENTRY(tf_pa_max_feedback[i]), string);
	}

	for(int i = 0; i < 6; i++) pa_min_vals[i] = min[i];
	for(int i = 0; i < 6; i++) pa_max_vals[i] = max[i];
}

G_MODULE_EXPORT void on_pa_switch_y_axis_type(GtkWidget* widget, gpointer data) {
	pa_yaxis_type = pa_yaxis_type != PA_YAXIS_DUR ? PA_YAXIS_DUR : PA_YAXIS_ARRDATE;
	update_pa();
}

G_MODULE_EXPORT void on_change_itin_group_visibility(GtkWidget* widget, gpointer data) {
	struct PorkchopGroup *group = (struct PorkchopGroup *) data;  // Cast data back to group struct
	int visibility = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	group->show_group = visibility;
	update_pa();
}

void update_group_overview() {
	int num_cols = 2;

	// Remove grid if exists
	if (grid_pa_groups != NULL && GTK_WIDGET(vp_pa_groups) == gtk_widget_get_parent(grid_pa_groups)) {
		gtk_container_remove(GTK_CONTAINER(vp_pa_groups), grid_pa_groups);
	}


	grid_pa_groups = gtk_grid_new();
	GtkWidget *separator;

	for (int col = 0; col < num_cols; col++) {
		// Create a horizontal separator line (optional)
		if(col > 0) {
			separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
			gtk_grid_attach(GTK_GRID(grid_pa_groups), separator, col * 2, 1, 1, 1);
		}

		char label_text[20];
		int req_width;
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

	// Create labels and buttons and add them to the grid
	for (int group_idx = 0; group_idx < pa_num_groups; group_idx++) {
		if(!pa_groups[group_idx].has_itin_inside_filter) continue;
		int row = group_idx*2+3;
		
		char widget_text[100];
		char tooltip_text[255];
		sprintf(widget_text, "");
		sprintf(tooltip_text, "");
		struct ItinStep *ptr;
		for(int step_idx = 0; step_idx < pa_groups[group_idx].num_steps; step_idx++) {
			ptr = pa_groups[group_idx].sample_arrival_node;
			for(int k = 0; k < pa_groups[group_idx].num_steps - step_idx - 1; k++) ptr = ptr->prev;
			if(step_idx != 0) {
				sprintf(widget_text, "%s - ", widget_text);
				sprintf(tooltip_text, "%s - ", tooltip_text);
			}
			if(ptr->body != NULL) {
				sprintf(widget_text, "%s%d", widget_text, get_body_system_id(ptr->body, pa_system) + 1);
				sprintf(tooltip_text, "%s%s", tooltip_text, ptr->body->name);
			} else
				sprintf(widget_text, "%sDSB", widget_text);
		}

		for (int j = 0; j < num_cols; j++) {
			int col = j*2+1;
			GtkWidget *widget = NULL;
			switch(j) {
				case 0:
					// Create a show group check button
					widget = gtk_check_button_new();
					gtk_widget_set_halign(widget, GTK_ALIGN_CENTER);
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), pa_groups[group_idx].show_group);
					g_signal_connect(widget, "clicked", G_CALLBACK(on_change_itin_group_visibility), &(pa_groups[group_idx]));
					pa_groups[group_idx].cb_pa_show_group = widget;
					gtk_widget_set_tooltip_text(widget, tooltip_text);
					break;
				case 1:
					// Create itinerary group label
					widget = gtk_label_new(widget_text);
					gtk_widget_set_tooltip_text(widget, tooltip_text);
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
			if(col > 1) {
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
	const struct PorkchopGroup *groupA = (const struct PorkchopGroup *) a;
	const struct PorkchopGroup *groupB = (const struct PorkchopGroup *) b;
	// Compare based on the count field
	return (groupB->count - groupA->count);
}

struct PorkchopGroup * find_itin_group(struct ItinStep *arrival) {
	struct ItinStep *ptr, *group_ptr;
	for(int i = 0; i < pa_num_groups; i++) {
		ptr = arrival;
		group_ptr = pa_groups[i].sample_arrival_node;
		while(group_ptr != NULL) {
			if(ptr == NULL) break;
			if(ptr->body != group_ptr->body) break;
			else {
				if(ptr->prev == NULL && group_ptr->prev == NULL) {
					return &pa_groups[i];
				}
			}
			group_ptr = group_ptr->prev;
			ptr = ptr->prev;
		}
	}
	return NULL;
}

void initialize_itinerary_groups() {
	int max_num_groups = 8;
	pa_groups = malloc(max_num_groups * sizeof(struct PorkchopGroup));
	pa_num_groups = 0;
	for(int i = 0; i < pa_num_itins; i++) {
		struct ItinStep *ptr, *group_ptr;
		int is_part_of_group = 0;
		for(int j = 0; j < pa_num_groups; j++) {
			ptr = pa_porkchop_points[i].data.arrival;
			group_ptr = pa_groups[j].sample_arrival_node;
			while(group_ptr != NULL) {
				if(ptr == NULL) break;
				if(ptr->body != group_ptr->body) break;
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
			if(pa_num_groups >= max_num_groups) {
				max_num_groups *= 2;
				struct PorkchopGroup *temp = realloc(pa_groups, max_num_groups * sizeof(struct PorkchopGroup));
				if(temp != NULL) pa_groups = temp;
				else {
					printf("Problem reallocating Porkchop Groups!!!\n");
					pa_num_groups--;
				}
			}
			pa_groups[pa_num_groups].sample_arrival_node = pa_porkchop_points[i].data.arrival;
			pa_groups[pa_num_groups].count = 1;
			pa_groups[pa_num_groups].num_steps = 1;
			pa_groups[pa_num_groups].show_group = 1;
			pa_groups[pa_num_groups].has_itin_inside_filter = 1;
			ptr = pa_porkchop_points[i].data.arrival;
			while(ptr->prev != NULL) {ptr = ptr->prev; pa_groups[pa_num_groups].num_steps++;}
			pa_num_groups++;
		}
	}

	qsort(pa_groups, pa_num_groups, sizeof(struct PorkchopGroup), compare_by_count);

	for(int i = 0; i < pa_num_itins; i++) pa_porkchop_points[i].group = find_itin_group(pa_porkchop_points[i].data.arrival);

	update_group_overview();
}

void update_best_itin() {
	if(pa_porkchop_points == NULL) return;
	int best_show_ind = 0;
	while(!pa_porkchop_points[best_show_ind].inside_filter || !pa_porkchop_points[best_show_ind].group->show_group) best_show_ind++;
	if(curr_transfer_pa != NULL) free_itinerary(get_first(curr_transfer_pa));
	curr_transfer_pa = create_itin_copy_from_arrival(pa_porkchop_points[best_show_ind].data.arrival);
	current_date_pa = get_last(curr_transfer_pa)->date;
	camera_zoom_to_fit_itinerary(pa_itin_preview_camera, curr_transfer_pa);
}

void analyze_departure_itins() {
	int num_itins = 0, tot_num_itins = 0;
	for(int i = 0; i < pa_num_deps; i++) num_itins += get_number_of_itineraries(pa_departures[i]);
	for(int i = 0; i < pa_num_deps; i++) tot_num_itins += get_total_number_of_stored_steps(pa_departures[i]);

	printf("\n%d itineraries found!\nNumber of Nodes: %d\n", num_itins, tot_num_itins);

	int index = 0;
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < pa_num_deps; i++) store_itineraries_in_array(pa_departures[i], arrivals, &index);
	pa_porkchop_points = malloc(num_itins * sizeof(struct PorkchopAnalyzerPoint));
	for(int i = 0; i < num_itins; i++) {
		pa_porkchop_points[i].data = create_porkchop_point(arrivals[i], get_first(arrivals[0])->body->atmo_alt + pa_dep_periapsis, arrivals[0]->body->atmo_alt + pa_arr_periapsis);
		pa_porkchop_points[i].inside_filter = 1;
		pa_porkchop_points[i].group = NULL;
	}
	free(arrivals);

	pa_num_itins = num_itins;
	initialize_itinerary_groups();
	sort_porkchop(pa_porkchop_points, pa_num_itins, pa_last_transfer_type);
	update_best_itin();
	reset_min_max_feedback(1);
}

G_MODULE_EXPORT void on_load_itineraries(GtkWidget* widget, gpointer data) {
	char filepath[255];
	if(!get_path_from_file_chooser(filepath, ".itins", GTK_FILE_CHOOSER_ACTION_OPEN, "")) return;

	free_all_porkchop_analyzer_itins();
	struct ItinsLoadFileResults load_results = load_itineraries_from_bfile(filepath);
	if(load_results.departures == NULL) return;
	
	if(pa_analysis_params.num_deps > 0 && pa_analysis_params.file_type > 2) {
		if(pa_analysis_params.calc_data.seq_info.to_target.type == ITIN_SEQ_INFO_TO_TARGET) free(pa_analysis_params.calc_data.seq_info.to_target.flyby_bodies);
		if(pa_analysis_params.calc_data.seq_info.spec_seq.type == ITIN_SEQ_INFO_SPEC_SEQ) free(pa_analysis_params.calc_data.seq_info.spec_seq.bodies);
	}
	pa_analysis_params = load_results.header;
	pa_num_deps = load_results.header.num_deps;
	pa_departures = load_results.departures;
	pa_system = load_results.header.system;
	update_camera_to_celestial_system(pa_itin_preview_camera, pa_system, deg2rad(90), 0);
	if(body_show_status_pa != NULL) free(body_show_status_pa);
	body_show_status_pa = (int*) calloc(pa_system->num_bodies, sizeof(int));
	analyze_departure_itins();

	update_pa_porkchop_diagram();
	pa_update_preview();
}

G_MODULE_EXPORT void on_save_best_itinerary(GtkWidget* widget, gpointer data) {
	struct ItinStep *first = get_first(curr_transfer_pa);
	if(first == NULL) return;

	char filepath[255];
	if(!get_path_from_file_chooser(filepath, ".itin", GTK_FILE_CHOOSER_ACTION_SAVE, "")) return;
	store_single_itinerary_in_bfile(first, pa_system, filepath);
}

G_MODULE_EXPORT void show_pa_analysis_parameters() {
	if(pa_analysis_params.num_deps == 0) return;
	char param_string[1000];
	print_header_data_to_string(pa_analysis_params, param_string, get_settings_datetime_type());
	show_msg_window(param_string);
}

G_MODULE_EXPORT void on_last_transfer_type_changed_pa(GtkWidget* widget, gpointer data) {
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) return;
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "fb") == 0) pa_last_transfer_type = TF_FLYBY;
	else if	(strcmp(name, "capture") == 0) pa_last_transfer_type = TF_CAPTURE;
	else if	(strcmp(name, "circ") == 0) pa_last_transfer_type = TF_CIRC;

	if(pa_porkchop_points == NULL) return;

	sort_porkchop(pa_porkchop_points, pa_num_itins, pa_last_transfer_type);
	update_best_itin();
	update_pa_porkchop_diagram();
	pa_update_preview();
	reset_min_max_feedback(1);
}

void update_pa() {
	update_best_itin();
	update_pa_porkchop_diagram();
	pa_update_preview();
	update_group_overview();
	reset_min_max_feedback(1);
}

gboolean are_any_porkchop_points_in_filter(const double min[6], const double max[6]) {
	// show only groups inside filter in gui (setting visible below)
	for(int group_idx = 0; group_idx < pa_num_groups; group_idx++) pa_groups[group_idx].has_itin_inside_filter = 0;

	int init_num_itins = 0;

	struct PorkchopPoint pp;
	for(int i = 0; i < pa_num_itins; i++) {
		if(pa_porkchop_points[i].inside_filter && pa_porkchop_points[i].group->show_group) init_num_itins++;
		pp = pa_porkchop_points[i].data;

		double dv_sat = pp.dv_dsm;
		if(pa_last_transfer_type == TF_CAPTURE)	dv_sat += pp.dv_arr_cap;
		if(pa_last_transfer_type == TF_CIRC)	dv_sat += pp.dv_arr_circ;

		if(	!(pp.dep_date 			< min[PA_DEP] || pp.dep_date 			> max[PA_DEP] ||
			  pp.dep_date+pp.dur 	< min[PA_ARR] || pp.dep_date+pp.dur 	> max[PA_ARR] ||
			  pp.dur 				< min[PA_DUR] || pp.dur 				> max[PA_DUR] ||
			  pp.dv_dep + dv_sat	< min[PA_TOTDV] || pp.dv_dep + dv_sat	> max[PA_TOTDV] ||
			  pp.dv_dep 			< min[PA_DEPDV] || pp.dv_dep 			> max[PA_DEPDV] ||
			  dv_sat 				< min[PA_SATDV] || dv_sat 				> max[PA_SATDV])) return TRUE;
	}
	return FALSE;
}

void apply_filter() {
	if(pa_porkchop_points == NULL) return;
	double min[6], max[6];
	char *string;
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_min_feedback[PA_DEP]));
	min[PA_DEP] = convert_date_JD(date_from_string(string, get_settings_datetime_type()))-1;	// rounding imprecision in filter entry field
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_max_feedback[PA_DEP]));
	max[PA_DEP] = convert_date_JD(date_from_string(string, get_settings_datetime_type()))+1;	// rounding imprecision in filter entry field
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_min_feedback[PA_ARR]));
	min[PA_ARR] = convert_date_JD(date_from_string(string, get_settings_datetime_type()))-1;	// rounding imprecision in filter entry field
	string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_max_feedback[PA_ARR]));
	max[PA_ARR] = convert_date_JD(date_from_string(string, get_settings_datetime_type()))+1;	// rounding imprecision in filter entry field
	for(int i = 2; i < 6; i++) {
		string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_min_feedback[i]));
		min[i] = strtod(string, NULL)-1;	// rounding imprecision in filter entry field
		string = (char*) gtk_entry_get_text(GTK_ENTRY(tf_pa_max_feedback[i]));
		max[i] = strtod(string, NULL)+1;	// rounding imprecision in filter entry field
		if(get_settings_datetime_type() == DATE_KERBAL && i == 1) {min[i] /= 4; max[i] /= 4;}
	}

	for(int i = 0; i < 6; i++) {
		if(min[i] > max[i]) {
			double temp = min[i];
			min[i] = max[i];
			max[i] = temp;
		}
	}

	if(!are_any_porkchop_points_in_filter(min, max)) return;

	// show only groups inside filter in gui (setting visible below)
	for(int group_idx = 0; group_idx < pa_num_groups; group_idx++) pa_groups[group_idx].has_itin_inside_filter = 0;

	int init_num_itins = 0, rem_num_itins = 0;

	struct PorkchopPoint pp;
	for(int i = 0; i < pa_num_itins; i++) {
		if(pa_porkchop_points[i].inside_filter && pa_porkchop_points[i].group->show_group) init_num_itins++;

		pa_porkchop_points[i].inside_filter = 1;
		pp = pa_porkchop_points[i].data;

		double dv_sat = pp.dv_dsm;
		if(pa_last_transfer_type == TF_CAPTURE)	dv_sat += pp.dv_arr_cap;
		if(pa_last_transfer_type == TF_CIRC)	dv_sat += pp.dv_arr_circ;

		if(	pp.dep_date 		< min[PA_DEP] 	|| pp.dep_date 			> max[PA_DEP] ||
			pp.dep_date+pp.dur 	< min[PA_ARR] 	|| pp.dep_date+pp.dur 	> max[PA_ARR] ||
			pp.dur 				< min[PA_DUR] 	|| pp.dur 				> max[PA_DUR] ||
			pp.dv_dep + dv_sat	< min[PA_TOTDV] || pp.dv_dep + dv_sat	> max[PA_TOTDV] ||
			pp.dv_dep 			< min[PA_DEPDV] || pp.dv_dep 			> max[PA_DEPDV] ||
			dv_sat 				< min[PA_SATDV] || dv_sat 				> max[PA_SATDV]) pa_porkchop_points[i].inside_filter = 0;
		else pa_porkchop_points[i].group->has_itin_inside_filter = 1;

		if(pa_porkchop_points[i].inside_filter && pa_porkchop_points[i].group->show_group) rem_num_itins++;
	}
}

G_MODULE_EXPORT void on_apply_filter(GtkWidget* widget, gpointer data) {
	if(pa_porkchop_points == NULL) return;
	apply_filter();
	reset_min_max_feedback(0);
	apply_filter();
	update_pa();
}

G_MODULE_EXPORT void on_pa_update(GtkWidget* widget, gpointer data) {
	if(pa_porkchop_points == NULL) return;
	reset_min_max_feedback(0);
	apply_filter();
	update_pa();
}

G_MODULE_EXPORT void on_reset_porkchop(GtkWidget* widget, gpointer data) {
	if(pa_porkchop_points == NULL) return;
	for(int i = 0; i < pa_num_itins; i++) pa_porkchop_points[i].inside_filter = 1;
	for(int i = 0; i < pa_num_groups; i++) {pa_groups[i].show_group = 1; pa_groups[i].has_itin_inside_filter = 1;}
	update_pa();
}

G_MODULE_EXPORT void on_prev_transfer_pa(GtkWidget* widget, gpointer data) {
	if(curr_transfer_pa == NULL) return;
	if(curr_transfer_pa->prev != NULL) curr_transfer_pa = curr_transfer_pa->prev;
	current_date_pa = curr_transfer_pa->date;
	pa_update_preview();
}

G_MODULE_EXPORT void on_next_transfer_pa(GtkWidget* widget, gpointer data) {
	if(curr_transfer_pa == NULL) return;
	if(curr_transfer_pa->next != NULL) curr_transfer_pa = curr_transfer_pa->next[0];
	current_date_pa = curr_transfer_pa->date;
	pa_update_preview();
}

G_MODULE_EXPORT void on_switch_steps_groups(GtkWidget* widget, gpointer data) {
	if(strcmp(gtk_stack_get_visible_child_name(GTK_STACK(st_pa_step_group_selector)), "page0") == 0)
		gtk_stack_set_visible_child_name(GTK_STACK(st_pa_step_group_selector), "page1");
	else
		gtk_stack_set_visible_child_name(GTK_STACK(st_pa_step_group_selector), "page0");
}

void end_porkchop_analyzer() {
	free_all_porkchop_analyzer_itins();
	destroy_camera(pa_itin_preview_camera);
	destroy_screen(pa_porkchop_screen);
}