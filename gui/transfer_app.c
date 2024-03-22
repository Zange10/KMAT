#include "transfer_app.h"
#include "tools/analytic_geometry.h"
#include "ephem.h"
#include "celestial_bodies.h"
#include "orbit_calculator/transfer_tools.h"
#include "drawing.h"
#include "orbit_calculator/orbit.h"

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <locale.h>


struct ItinStep *curr_transfer;

static int counter = 0;
GObject *drawing_area;
GObject *lb_date;
GObject *tb_tfdate;
GObject *bt_tfbody;
GObject *lb_transfer_dv;
GObject *lb_total_dv;
GObject *transfer_panel;

gboolean body_show_status[9];
struct Ephem **ephems;

double current_date;

void update();
void update_date_label();
void update_transfer_panel();
void remove_all_transfers();
void activate(GtkApplication *app, gpointer user_data);
void on_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
void on_body_toggle(GtkWidget* widget, gpointer data);
void on_change_date(GtkWidget* widget, gpointer data);
void on_calendar_selection(GtkWidget* widget, gpointer data);
void on_prev_transfer(GtkWidget* widget, gpointer data);
void on_next_transfer(GtkWidget* widget, gpointer data);
void on_transfer_body_change(GtkWidget* widget, gpointer data);
void on_toggle_transfer_date_lock(GtkWidget* widget, gpointer data);
void on_goto_transfer_date(GtkWidget* widget, gpointer data);
void on_transfer_body_select(GtkWidget* widget, gpointer data);
void on_add_transfer(GtkWidget* widget, gpointer data);
void on_remove_transfer(GtkWidget* widget, gpointer data);
void on_find_closest_transfer(GtkWidget* widget, gpointer data);
void on_find_itinerary(GtkWidget* widget, gpointer data);



void start_transfer_app() {
	int num_bodies = 9;
	int num_ephems = 12*100;	// 12 months for 100 years (1950-2050)
	
	ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
	for(int i = 0; i < num_bodies; i++) {
		ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
		get_body_ephem(ephems[i], i+1);
	}
	struct Date date = {1977, 8, 20, 0, 0, 0};
	current_date = convert_date_JD(date);
	for(int i = 0; i < 9; i++) body_show_status[i] = 0;
	
	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	
	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
	
	remove_all_transfers();
	for(int i = 0; i < 9; i++) free(ephems[i]);
	free(ephems);
}

void activate(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/transfer.glade", NULL);
	
	gtk_builder_connect_signals(builder, NULL);
	
	tb_tfdate = gtk_builder_get_object(builder, "tb_tfdate");
	bt_tfbody = gtk_builder_get_object(builder, "bt_change_tf_body");
	lb_transfer_dv = gtk_builder_get_object(builder, "lb_transfer_dv");
	lb_total_dv = gtk_builder_get_object(builder, "lb_total_dv");
	transfer_panel = gtk_builder_get_object(builder, "transfer_panel");
	lb_date = gtk_builder_get_object(builder, "lb_date");
	update_date_label();
	update_transfer_panel();
	
	
	drawing_area = gtk_builder_get_object(builder, "drawing_area");
	g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
	
	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);
	
	/* We do not need the builder anymore */
	g_object_unref(builder);
}


struct ItinStep * get_first() {
	if(curr_transfer == NULL) return NULL;
	struct ItinStep *tf = curr_transfer;
	while(tf->prev != NULL) tf = tf->prev;
	return tf;
}

struct ItinStep * get_last() {
	if(curr_transfer == NULL) return NULL;
	struct ItinStep *tf = curr_transfer;
	while(tf->next != NULL) tf = tf->next[0];
	return tf;
}


void on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
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
	for(int i = 0; i < 9; i++) if(body_show_status[i]) highest_id = i+1;
	double scale = calc_scale(area_width, area_height, highest_id);
	
	// Sun
	set_cairo_body_color(cr, 0);
	draw_body(cr, center, 0, vec(0,0,0));
	cairo_fill(cr);
	
	// Planets
	for(int i = 0; i < 9; i++) {
		if(body_show_status[i]) {
			int id = i+1;
			set_cairo_body_color(cr, id);
			struct OSV osv = osv_from_ephem(ephems[i], current_date, SUN());
			draw_body(cr, center, scale, osv.r);
			draw_orbit(cr, center, scale, osv.r, osv.v, SUN());
		}
	}
	
	// Transfers
	if(curr_transfer != NULL) {
		struct ItinStep *temp_transfer = get_first();
		while(temp_transfer != NULL) {
			if(temp_transfer->body != NULL) {
				int id = temp_transfer->body->id;
				set_cairo_body_color(cr, id);
				draw_transfer_point(cr, center, scale, temp_transfer->r);
			}
			if(temp_transfer->prev != NULL) draw_trajectory(cr, center, scale, temp_transfer);
			temp_transfer = temp_transfer->next != NULL ? temp_transfer->next[0] : NULL;
		}
	}
	
	update_transfer_panel();	// Seems redundant, but necessary for updating dv numbers
}

double calc_total_dv() {
	if(curr_transfer == NULL) return 0;
	struct ItinStep *temp_transfer = get_last();
	return 0;
	double porkchop[5];
	create_porkchop_point(temp_transfer, porkchop);
	return porkchop[2]+porkchop[3]+porkchop[4];
}

//int find_closest_transfer_from_transfer(struct ItinStep *transfer) {
//	if(transfer->prev == NULL || transfer->prev->prev == NULL) return 1;
//	struct TransferData *tf[3] = {transfer->prev->prev, transfer->prev, transfer};
//	double t[3];
//	struct OSV osvs[3];
//	struct Body *bodies[3];
//
//	for(int i = 0; i < 3; i++) {
//		t[i] = tf[i]->date;
//		osvs[i] = osv_from_ephem(ephems[tf[i]->body->id-1], tf[i]->date, SUN());
//		bodies[i] = tf[i]->body;
//	}
//	double closest_transfer = find_closest_transfer(t, osvs, bodies, ephems, 10*365);
//	if(closest_transfer < 0) return 0;
//	current_date = closest_transfer;
//	transfer->date = closest_transfer;
//	return 1;
//}

void update_itinerary() {
	update_itin_body_osvs(get_first(), ephems);
	calc_itin_v_vectors_from_dates_and_r(get_first());
	update();
}

void sort_transfer_dates() {
	struct ItinStep *tf = get_first();
	if(tf == NULL) return;
	while(tf->next != NULL) {
		if(tf->next[0]->date <= tf->date) tf->next[0]->date = tf->date+1;
		tf = tf->next[0];
	}
	update_itinerary();
}

void remove_all_transfers() {
	free_itinerary(get_first());
	curr_transfer = NULL;
}

void update() {
	update_date_label();
	update_transfer_panel();
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}

void update_date_label() {
	char date_string[10];
	date_to_string(convert_JD_date(current_date), date_string, 0);
	gtk_label_set_text(GTK_LABEL(lb_date), date_string);
}

void update_transfer_panel() {
	if(curr_transfer == NULL) {
		gtk_button_set_label(GTK_BUTTON(tb_tfdate), "0000-00-00");
		gtk_button_set_label(GTK_BUTTON(bt_tfbody), "Planet");
	} else {
		struct Date date = convert_JD_date(curr_transfer->date);
		char date_string[10];
		date_to_string(date, date_string, 0);
		gtk_button_set_label(GTK_BUTTON(tb_tfdate), date_string);
		if(curr_transfer->body != NULL) gtk_button_set_label(GTK_BUTTON(bt_tfbody), curr_transfer->body->name);
		else gtk_button_set_label(GTK_BUTTON(bt_tfbody), "Deep-Space Man");
		char s_dv[20];
		sprintf(s_dv, "%6.0f m/s", 0.0);
		gtk_label_set_label(GTK_LABEL(lb_transfer_dv), s_dv);
		sprintf(s_dv, "%6.0f m/s", calc_total_dv());
		gtk_label_set_label(GTK_LABEL(lb_total_dv), s_dv);
	}
}


void on_body_toggle(GtkWidget* widget, gpointer data) {
	int id = (int) gtk_widget_get_name(widget)[0] - 48;	// char to int
	body_show_status[id-1] = body_show_status[id-1] ? 0 : 1;
	gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}


void on_change_date(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	if		(strcmp(name, "+1Y") == 0) current_date = jd_change_date(current_date, 1, 0, 0);
	else if	(strcmp(name, "+1M") == 0) current_date = jd_change_date(current_date, 0, 1, 0);
	else if	(strcmp(name, "+1D") == 0) current_date++;
	else if	(strcmp(name, "-1Y") == 0) current_date = jd_change_date(current_date,-1, 0, 0);
	else if	(strcmp(name, "-1M") == 0) current_date = jd_change_date(current_date, 0,-1, 0);
	else if	(strcmp(name, "-1D") == 0) current_date--;

	if(curr_transfer != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tfdate))) {
		if(curr_transfer->prev != NULL && current_date <= curr_transfer->prev->date) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
			return;
		}
		curr_transfer->date = current_date;
		sort_transfer_dates();
	}

	update_itinerary();
}


void on_year_select(GtkWidget* widget, gpointer data) {
	const char *name = gtk_widget_get_name(widget);
	int year = atoi(name);
	struct Date date = {year, 1,1};
	current_date = convert_date_JD(date);
	update_itinerary();
}



void on_prev_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL) return;
	if(curr_transfer->prev != NULL) curr_transfer = curr_transfer->prev;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
	current_date = curr_transfer->date;
	update();
}

void on_next_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL) return;
	if(curr_transfer->next != NULL) curr_transfer = curr_transfer->next[0];
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
	current_date = curr_transfer->date;
	update();
}

void on_transfer_body_change(GtkWidget* widget, gpointer data) {
	if(curr_transfer==NULL) return;
	gtk_stack_set_visible_child_name(GTK_STACK(transfer_panel), "page1");
	update_itinerary();
}

void on_toggle_transfer_date_lock(GtkWidget* widget, gpointer data) {
	if(curr_transfer != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb_tfdate)))
		current_date = curr_transfer->date;
	update_itinerary();
}

void on_goto_transfer_date(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL) return;
	current_date = curr_transfer->date;
	update_itinerary();
}

void on_transfer_body_select(GtkWidget* widget, gpointer data) {
	int id = (int) gtk_widget_get_name(widget)[0] - 48;	// char to int
	if(id == 0) {
		curr_transfer->body = NULL;
	} else {
		struct Body *body = get_body_from_id(id);
		curr_transfer->body = body;
	}
	gtk_stack_set_visible_child_name(GTK_STACK(transfer_panel), "page0");
	update_itinerary();
}

void on_add_transfer(GtkWidget* widget, gpointer data) {
	struct ItinStep *new_transfer = (struct ItinStep *) malloc(sizeof(struct ItinStep));
	new_transfer->body = EARTH();
	new_transfer->prev = NULL;
	new_transfer->next = NULL;
	new_transfer->num_next_nodes = 0;

	struct ItinStep *next = (curr_transfer != NULL && curr_transfer->next != NULL) ? curr_transfer->next[0] : NULL;

	// date
	new_transfer->date = current_date;
	if(curr_transfer != NULL) {
		if(current_date <= curr_transfer->date) new_transfer->date = curr_transfer->date + 1;
		if(next != NULL) {
			if(next->date - curr_transfer->date < 2) {
				free(new_transfer);
				return;    // no space between this and next transfer...
			}
			if(new_transfer->date >= next->date) new_transfer->date = next->date - 1;
		}
	}

	// next and prev pointers
	if(curr_transfer != NULL) {
		if(next != NULL) {
			next->prev = new_transfer;
			new_transfer->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
			new_transfer->next[0] = next;
			new_transfer->num_next_nodes = 1;
		} else {
			curr_transfer->next = (struct ItinStep**) malloc(sizeof(struct ItinStep*));
			curr_transfer->num_next_nodes = 1;
		}
		new_transfer->prev = curr_transfer;
		curr_transfer->next[0] = new_transfer;
	}

	// Location and Velocity vectors
	update_itinerary();

	curr_transfer = new_transfer;
	current_date = curr_transfer->date;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
	update_itinerary();
}

void on_remove_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL) return;
	struct ItinStep *rem_transfer = curr_transfer;
	if(curr_transfer->next != NULL) curr_transfer->next[0]->prev = curr_transfer->prev;
	if(curr_transfer->prev != NULL) {
		if(curr_transfer->next != NULL) {
			curr_transfer->prev->next[0] = curr_transfer->next[0];
		} else {
			free(curr_transfer->prev->next);
			curr_transfer->prev->next = NULL;
			curr_transfer->prev->num_next_nodes = 0;
		}
	}

	if(curr_transfer->prev != NULL) 		curr_transfer = curr_transfer->prev;
	else if(curr_transfer->next != NULL) 	curr_transfer = curr_transfer->next[0];
	else 									curr_transfer = NULL;

	free(rem_transfer->next);
	free(rem_transfer);
	if(curr_transfer != NULL) current_date = curr_transfer->date;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_tfdate), 0);
	update();
}



void on_find_closest_transfer(GtkWidget* widget, gpointer data) {
	if(curr_transfer == NULL || curr_transfer->prev == NULL || curr_transfer->prev->prev == NULL) return;
	struct ItinStep *prev = curr_transfer->prev;
	struct ItinStep *temp = (struct ItinStep*) malloc(sizeof(struct ItinStep));
	temp->body = prev->body;
	temp->r = prev->r;
	temp->v_body = prev->v_body;
	temp->v_arr = prev->v_arr;
	temp->v_dep = prev->v_dep;
	temp->date = prev->date;
	temp->next = NULL;
	temp->prev = NULL;
	temp->num_next_nodes = 0;
	find_viable_flybys(temp, ephems[curr_transfer->body->id-1], curr_transfer->body, 86400, 86400*365.25*50);

	if(temp->next != NULL) {
		struct ItinStep *new_step = temp->next[0];
		curr_transfer->r = new_step->r;
		curr_transfer->v_body = new_step->v_body;
		curr_transfer->v_dep = new_step->v_dep;
		curr_transfer->v_arr = new_step->v_arr;
		curr_transfer->date = new_step->date;
		curr_transfer->v_body = new_step->v_body;
		for(int i = 0; i < temp->num_next_nodes; i++) free(temp->next[i]);
		free(temp->next);
		current_date = curr_transfer->date;
	}

	sort_transfer_dates();

	free(temp);
	update_itinerary();
}

void on_find_itinerary(GtkWidget* widget, gpointer data) {

}
