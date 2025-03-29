#include <stdio.h>

#include "drawing.h"
#include "tools/datetime.h"
#include "settings.h"
#include "math.h"


void draw_orbit(cairo_t *cr, struct Vector2D center, double scale, struct Vector r, struct Vector v, struct Body *attractor) {
	int steps = 100;
	struct OSV last_osv = {r,v};
	
	for(int i = 1; i <= steps; i++) {
		double theta = 2*M_PI/steps * i;
		struct OSV osv = propagate_orbit_theta(constr_orbit_from_osv(r,v,attractor),theta,attractor);
		// y negative, because in GUI y gets bigger downwards
		struct Vector2D p1 = {last_osv.r.x, -last_osv.r.y};
		struct Vector2D p2 = {osv.r.x, -osv.r.y};
		
		p1 = scalar_multipl2d(p1,scale);
		p2 = scalar_multipl2d(p2,scale);
		
		draw_stroke(cr, add_vectors2d(p1,center), add_vectors2d(p2,center));
		last_osv = osv;
	}
}

void draw_body(cairo_t *cr, struct Vector2D center, double scale, struct Vector r) {
	int body_radius = 5;
	r = scalar_multiply(r, scale);
	// y negative, because in GUI y gets bigger downwards
	r.y *= -1;
	cairo_arc(cr, center.x+r.x, center.y+r.y,body_radius,0,M_PI*2);
	cairo_fill(cr);
}

void draw_transfer_point(cairo_t *cr, struct Vector2D center, double scale, struct Vector r) {
	int cross_length = 4;
	cairo_set_source_rgb(cr, 1, 0, 0);
	r = scalar_multiply(r, scale);
	// y negative, because in GUI y gets bigger downwards
	r.y *= -1;
	for(int i = -1; i<=1; i+=2) {
		struct Vector2D p1 = {r.x - cross_length, r.y + cross_length*i};
		struct Vector2D p2 = {r.x + cross_length, r.y - cross_length*i};
		draw_stroke(cr, add_vectors2d(center, p1), add_vectors2d(center, p2));
	}
}




// Rework trajectory drawing -> OSV + dt   ----------------------------------------------
void draw_trajectory(cairo_t *cr, struct Vector2D center, double scale, struct ItinStep *tf, struct Body *attractor) {
	// if double swing-by is not worth drawing
	if(tf->body == NULL && tf->v_body.x == 0) return;

	cairo_set_source_rgb(cr, 0, 1, 0);
	struct ItinStep *prev = tf->prev;
	// skip not working double swing-by
	if(tf->prev->body == NULL && tf->prev->v_body.x == 0) prev = tf->prev->prev;
	double dt = (tf->date-prev->date)*24*60*60;

	if(prev->prev != NULL && prev->body != NULL) {
		double t[3] = {prev->prev->date, prev->date, tf->date};
		struct OSV osv0 = {prev->prev->r, prev->prev->v_body};
		struct OSV osv1 = {prev->r, prev->v_body};
		struct OSV osv2 = {tf->r, tf->v_body};
		struct OSV osvs[3] = {osv0, osv1, osv2};
		struct Body *bodies[3] = {prev->prev->body, prev->body, tf->body};
		if(!is_flyby_viable(t, osvs, bodies, attractor)) cairo_set_source_rgb(cr, 1, 0, 0);
	}

	int steps = 1000;
	struct Vector r = prev->r;
	struct Vector v = tf->v_dep;
	struct OSV last_osv = {r,v};

	for(int i = 1; i <= steps; i++) {
		double time = dt/steps * i;
		struct OSV osv = propagate_orbit_time(constr_orbit_from_osv(r,v,attractor),time, attractor);
		// y negative, because in GUI y gets bigger downwards
		struct Vector2D p1 = {last_osv.r.x, -last_osv.r.y};
		struct Vector2D p2 = {osv.r.x, -osv.r.y};
		
		p1 = scalar_multipl2d(p1,scale);
		p2 = scalar_multipl2d(p2,scale);
		
		draw_stroke(cr, add_vectors2d(p1,center), add_vectors2d(p2,center));
		last_osv = osv;
	}
}


void draw_stroke(cairo_t *cr, struct Vector2D p1, struct Vector2D p2) {
	cairo_set_line_width(cr, 1);
	cairo_move_to(cr, p1.x, p1.y);
	cairo_line_to(cr, p2.x, p2.y);
	cairo_stroke(cr);
}

double calc_scale(int area_width, int area_height, struct Body *farthest_body) {
	if(farthest_body == NULL) return 1e-9;
	double apoapsis = farthest_body->orbit.apoapsis;
	int wh = area_width < area_height ? area_width : area_height;
	return 1/apoapsis*wh/2.2;	// divided by 2.2 because apoapsis is only one side and buffer
}

void set_cairo_body_color(cairo_t *cr, struct Body *body) {
	if(body != NULL) {
		cairo_set_source_rgb(cr, body->color[0], body->color[1], body->color[2]);
	} else cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
}

void draw_center_aligned_text(cairo_t *cr, double x, double y, char *text) {
	// Calculate text width
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	double text_width = extents.width;

	// Position and draw text to the left
	cairo_move_to(cr, x - text_width/2, y); // adjust starting position to the left
	cairo_show_text(cr, text);
}

void draw_right_aligned_text(cairo_t *cr, double x, double y, char *text) {
	// Calculate text width
	cairo_text_extents_t extents;
	cairo_text_extents(cr, text, &extents);
	double text_width = extents.width;

	// Position and draw text to the left
	cairo_move_to(cr, x - text_width, y); // adjust starting position to the left
	cairo_show_text(cr, text);
}

void draw_data_point(cairo_t *cr, double x, double y, double radius) {
	cairo_arc(cr, x, y, radius,0,M_PI*2);
	cairo_fill(cr);
}

void draw_coordinate_system(cairo_t *cr, double width, double height, enum CoordAxisLabelType x_axis_label_type, enum CoordAxisLabelType y_axis_label_type,
		double min_x, double max_x, double min_y, double max_y, struct Vector2D origin, int num_x_labels, int num_y_labels) {
	// Set text color
	cairo_set_source_rgb(cr, 1, 1, 1);

	// Set font options
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12.0);
	int half_font_size = 5;

	// axes
	draw_stroke(cr, vec2D(origin.x, 0), vec2D(origin.x, origin.y));
	draw_stroke(cr, vec2D(origin.x, origin.y), vec2D(width, origin.y));

	const double tick_units[] = {1,2,5};
	double y_label_tick = tick_units[0];
	double x_label_tick = tick_units[0];
	double min_x_label, min_y_label;
	double tick_scale = 1e-9;

	// x tick size and min label
	if(x_axis_label_type == COORD_LABEL_NUMBER) {
		double tick_unit;
		do {
			for(int i = 0; i < sizeof(tick_units)/sizeof(tick_units[0]); i++) {
				tick_unit = tick_units[i];
				x_label_tick = tick_unit * tick_scale;
				if(num_x_labels * x_label_tick > (max_x - min_x)) break;
			}
			tick_scale *= 10;
		} while(num_x_labels * x_label_tick < (max_x - min_x));
		tick_scale = 1e-9;
		min_x_label = (min_x == 0) ? x_label_tick : ceil(min_x/x_label_tick)*x_label_tick;
	} else if(x_axis_label_type == COORD_LABEL_DATE) {
		min_x_label = floor(min_x+2.0/3*(max_x-min_x)/num_x_labels);
		x_label_tick = ceil((max_x-min_x)/num_x_labels);
	}

	// x tick size and min label
	if(y_axis_label_type == COORD_LABEL_NUMBER) {
		double tick_unit;
		do {
			for(int i = 0; i < sizeof(tick_units)/sizeof(tick_units[0]); i++) {
				tick_unit = tick_units[i];
				y_label_tick = tick_unit * tick_scale;
				if(num_y_labels * y_label_tick > (max_y - min_y)) break;
			}
			tick_scale *= 10;
		} while(num_y_labels * y_label_tick < (max_y - min_y));
		min_y_label = (min_y == 0) ? y_label_tick : ceil(min_y/y_label_tick)*y_label_tick;
	} else if(x_axis_label_type == COORD_LABEL_DATE) {
		min_y_label = floor(min_y+2.0/3*(max_y-min_y)/num_y_labels);
		y_label_tick = ceil((max_y-min_y)/num_y_labels);
	}

	// gradients
	double m_y, m_x;
	m_x = (width-origin.x)/(max_x - min_x);
	m_y = -origin.y/(max_y - min_y); // negative, because positive is down

	double y_label_x = origin.x-5,	x_label_y = origin.y + 20;

	// x-labels and x grid
	char string[32];
	for(int i = 0; i < num_x_labels; i++) {
		double label = min_x_label + i * x_label_tick;
		double x = (label-min_x)*m_x + origin.x;
		cairo_set_source_rgb(cr, 1, 1, 1);
		if(x_axis_label_type == COORD_LABEL_NUMBER)
			sprintf(string, "%g", label);
		else if(x_axis_label_type == COORD_LABEL_DURATION)
			sprintf(string, "%g", get_settings_datetime_type() == DATE_KERBAL ? label*4 : label);
		else if(x_axis_label_type == COORD_LABEL_DATE)
			date_to_string(convert_JD_date(label, get_settings_datetime_type()), string, 0);
		draw_center_aligned_text(cr, x, x_label_y, string);
		cairo_set_source_rgb(cr, 0, 0, 0);
		draw_stroke(cr, vec2D(x, origin.y), vec2D(x, 0));
	}

	// y-labels and y grid
	for(int i = 0; i < num_y_labels; i++) {
		double label = min_y_label + i * y_label_tick;
		double y = (label-min_y)*m_y + origin.y;
		cairo_set_source_rgb(cr, 1, 1, 1);
		if(y_axis_label_type == COORD_LABEL_NUMBER)
			sprintf(string, "%g", label);
		else if(y_axis_label_type == COORD_LABEL_DURATION)
			sprintf(string, "%g", get_settings_datetime_type() == DATE_KERBAL ? label*4 : label);
		else if(y_axis_label_type == COORD_LABEL_DATE)
			date_to_string(convert_JD_date(label, get_settings_datetime_type()), string, 0);
		draw_right_aligned_text(cr, y_label_x, y+half_font_size, string);
		cairo_set_source_rgb(cr, 0, 0, 0);
		draw_stroke(cr, vec2D(origin.x, y), vec2D(width, y));
	}
}

void draw_porkchop(cairo_t *cr, double width, double height, struct PorkchopAnalyzerPoint *porkchop, int num_itins, enum LastTransferType last_transfer_type) {
	double dv, date, dur;

	struct Vector2D origin = {45, height-40};

	int first_show_ind = 0;
	while(!porkchop[first_show_ind].inside_filter) first_show_ind++;

	struct PorkchopPoint pp = porkchop[first_show_ind].data;
	double dv_sat = pp.dv_dsm;
	if(last_transfer_type == TF_CAPTURE)	dv_sat += pp.dv_arr_cap;
	if(last_transfer_type == TF_CIRC)		dv_sat += pp.dv_arr_circ;

	double min_date = pp.dep_date, max_date = pp.dep_date;
	double min_dur = pp.dur, max_dur = pp.dur;
	double min_dv = pp.dv_dep + dv_sat;
	double max_dv = pp.dv_dep + dv_sat;

	// find min and max
	for(int i = 1; i < num_itins; i++) {
		if(!porkchop[i].inside_filter) continue;
		pp = porkchop[i].data;

		dv_sat = pp.dv_dsm;
		if(last_transfer_type == TF_CAPTURE)	dv_sat += pp.dv_arr_cap;
		if(last_transfer_type == TF_CIRC)		dv_sat += pp.dv_arr_circ;

		dv = pp.dv_dep + dv_sat;
		date = pp.dep_date;
		dur = pp.dur;

		if(dv < min_dv) min_dv = dv;
		else if(dv > max_dv) max_dv = dv;
		if(date < min_date) min_date = date;
		else if(date > max_date) max_date = date;
		if(dur < min_dur) min_dur = dur;
		else if(dur > max_dur) max_dur = dur;
	}


	double ddate = max_date-min_date;
	double ddur = max_dur - min_dur;

	double margin = 0.05;

	min_date = min_date-ddate*margin;
	max_date = max_date+ddate*margin;
	min_dur = min_dur - ddur * margin;
	max_dur = max_dur + ddur * margin;

	// gradients
	double m_dur, m_date;
	m_date = (width-origin.x)/(max_date - min_date);
	m_dur = -origin.y/(max_dur - min_dur); // negative, because positive is down

	draw_coordinate_system(cr, width, height, COORD_LABEL_DATE, COORD_LABEL_DURATION, min_date, max_date, min_dur, max_dur, origin, 5, 10);

	// find points to draw
	int *draw_idx = calloc(sizeof(int), num_itins);
	int num_draw_itins = 0;
	for(int i = 0; i < num_itins; i++) {
		if(porkchop[i].inside_filter && porkchop[i].group->show_group) {
			draw_idx[num_draw_itins] = i;
			num_draw_itins++;
		}
	}
	// draw points
	double color_bias;
	int i = num_draw_itins-1;
	while(i >= 0) {
		pp = porkchop[draw_idx[i]].data;

		dv_sat = pp.dv_dsm;
		if(last_transfer_type == TF_CAPTURE)	dv_sat += pp.dv_arr_cap;
		if(last_transfer_type == TF_CIRC)		dv_sat += pp.dv_arr_circ;

		dv = pp.dv_dep + dv_sat;
		date = pp.dep_date;
		dur = pp.dur;

		// color coding
		color_bias = (dv - min_dv) / (max_dv - min_dv);
		double r = i == 0 ? 1 : color_bias;
		double g = i == 0 ? 0 : 1-color_bias;
		double b = i == 0 ? 0 : 4*pow(color_bias-0.5,2);
		cairo_set_source_rgb(cr, r,g,b);

		struct Vector2D data_point = vec2D(origin.x + m_date*(date - min_date), origin.y + m_dur * (dur - min_dur));
		draw_data_point(cr, data_point.x, data_point.y, i > 0 ? 2 : 5);

		if(num_draw_itins > 100000) {
			if(i > 1e6) i -= 50;
			else if(i > 1e5) i -= 10;
			else if(i > 1e4) i -= 5;
			else if(i > 1e2) i -= 2;
			else i--;
		} else {
			i--;
		}
	}

	free(draw_idx);
}

void draw_plot(cairo_t *cr, double width, double height, double *x, double *y, int num_points) {
	struct Vector2D origin = {60, height-30};

	double min_x = x[0], max_x = x[0];
	double min_y = y[0], max_y = y[0];
	if(min_y > 0) min_y = 0;

	// find min and max
	for(int i = 1; i < num_points; i++) {
		if(x[i] < min_x) min_x = x[i];
		else if(x[i] > max_x) max_x = x[i];
		if(y[i] < min_y) min_y = y[i];
		else if(y[i] > max_y) max_y = y[i];
	}

	double dx = max_x-min_x;
	double dy = max_y-min_y;


	double margin = 0.05;

	//min_x = min_x != 0 ? min_x - dx * margin : 0;
	//max_x = max_x != 0 ? max_x + dx * margin : 0;
	//dx = max_x-min_x;
	min_y = min_y != 0 ? min_y - dy * margin : 0;
	max_y = max_y != 0 ? max_y + dy * margin : 0;
	dy = max_y-min_y;

	// gradients
	double m_x, m_y;
	m_x = (width-origin.x)/dx;
	m_y = -origin.y/dy; // negative, because positive is down

	draw_coordinate_system(cr, width, height, COORD_LABEL_NUMBER, COORD_LABEL_NUMBER, min_x, max_x, min_y, max_y, origin, 10, 15);

	// data
	cairo_set_source_rgb(cr, 0, 0.8, 0.8);
	for(int i = 1; i < num_points; i++) {
		struct Vector2D point0 = vec2D(origin.x + m_x*(x[i-1] - min_x), origin.y + m_y * (y[i-1] - min_y));
		struct Vector2D point1 = vec2D(origin.x + m_x*(x[i  ] - min_x), origin.y + m_y * (y[i  ] - min_y));
		draw_stroke(cr, point0, point1);
	}
}

void draw_multi_plot(cairo_t *cr, double width, double height, double *x, double **y, int num_plots, int num_points) {
	struct Vector2D origin = {60, height-30};

	struct Vector colors[2] = {
			vec(0, 0.8, 0.8),
			vec(0.8, 0, 0.4),
	};

	double min_x = x[0], max_x = x[0];
	double min_y = y[0][0], max_y = y[0][0];
	if(min_y > 0) min_y = 0;

	// find min and max
	for(int i = 1; i < num_points; i++) {
		if(x[i] < min_x) min_x = x[i];
		else if(x[i] > max_x) max_x = x[i];
	}

	for(int p = 0; p < num_plots; p++) {
		for(int i = 0; i < num_points; i++) {
			if(y[p][i] < min_y) min_y = y[p][i];
			else if(y[p][i] > max_y) max_y = y[p][i];
		}
	}

	double dx = max_x-min_x;
	double dy = max_y-min_y;


	double margin = 0.05;

	//min_x = min_x != 0 ? min_x - dx * margin : 0;
	//max_x = max_x != 0 ? max_x + dx * margin : 0;
	//dx = max_x-min_x;
	min_y = min_y != 0 ? min_y - dy * margin : 0;
	max_y = max_y != 0 ? max_y + dy * margin : 0;
	dy = max_y-min_y;

	// gradients
	double m_x, m_y;
	m_x = (width-origin.x)/dx;
	m_y = -origin.y/dy; // negative, because positive is down

	draw_coordinate_system(cr, width, height, COORD_LABEL_NUMBER, COORD_LABEL_NUMBER, min_x, max_x, min_y, max_y, origin, 10, 15);

	// data
	for(int p = 0; p < num_plots; p++) {
		cairo_set_source_rgb(cr, colors[p].x, colors[p].y, colors[p].z);
		for(int i = 1; i < num_points; i++) {
			struct Vector2D point0 = vec2D(origin.x + m_x * (x[i - 1] - min_x), origin.y + m_y * (y[p][i - 1] - min_y));
			struct Vector2D point1 = vec2D(origin.x + m_x * (x[i] - min_x), origin.y + m_y * (y[p][i] - min_y));
			draw_stroke(cr, point0, point1);
		}
	}
}
