#include "coordinate_system_drawing.h"
#include "gui/drawing.h"
#include "gui/settings.h"
#include <math.h>


void get_coordinate_system_axis_ticks(CSAxisLabelType axis_label_type, int num_labels, double min_value, double max_value, double *p_value_tick, Datetime *p_date_tick, double *p_min_label) {
	if(axis_label_type == CS_AXIS_NUMBER || axis_label_type == CS_AXIS_DURATION) {
		double tick_units[] = {1,2,5};
		double tick = tick_units[0];
		double tick_scale = 1e-9;
		do {
			for(int i = 0; i < sizeof(tick_units)/sizeof(tick_units[0]); i++) {
				tick = tick_units[i] * tick_scale;
				if(num_labels*tick > (max_value - min_value)) break;
			}
			tick_scale *= 10;
		} while(num_labels*tick < (max_value - min_value));
		*p_value_tick = tick;
		*p_min_label = (min_value == 0) ? tick : ceil(min_value/tick)*tick;
	} else if(axis_label_type == CS_AXIS_DATE) {
		Datetime tick = {0, 0, 1};
		double min_label = 0;
		for(int i = 0; i < 3; i++) {
			if(get_settings_datetime_type() != DATE_ISO && i == 1) continue; // kerbal date doesn't have months
			tick = i == 0 ? (Datetime){0, 0, 1} : i == 1 ? (Datetime) {0, 1, 0} : (Datetime) {1, 0, 0};
			int tick_scale = 0;
			bool found_valid_tick = false;
			do {
				switch(tick_scale) {
					case 0: tick_scale = 1; break;
					case 1: tick_scale = 2; break;
					case 2: tick_scale = i!=1 ? 5 : 6; break;
					case 5: tick_scale = 10; break;
					case 10: tick_scale = 20; break;
					case 20: tick_scale = 50; break;
					case 50: tick_scale = 100; break;
					default: tick_scale *= 2;
				}
				tick = (Datetime) {(tick.y != 0)*tick_scale, (tick.m != 0)*tick_scale, (tick.d != 0)*tick_scale};
				Datetime min_date = convert_JD_date(min_value, get_settings_datetime_type());
				Datetime min_x_date_label = (Datetime) {
						min_date.y,
						tick.y == 0 && tick.m <6 ? min_date.m : 1,
										(tick.y == 0 && tick.m == 0) ? min_date.d : 1,
								.date_type = get_settings_datetime_type()
				};
				min_label = convert_date_JD(min_x_date_label);
				min_label = jd_change_date(
						min_label,
						tick.y,
						tick.m,
						tick.d,
						get_settings_datetime_type()
				);
				double max_label = jd_change_date(
						min_label,
						tick.y*num_labels,
						tick.m*num_labels,
						tick.d*num_labels,
						get_settings_datetime_type()
				);
				if(max_label > max_value) { found_valid_tick = true; break;}
			} while((tick.d < 10 || get_settings_datetime_type() != DATE_ISO) && tick.d < 100 && tick.m < 6);
			if(found_valid_tick) break;
		}
		*p_date_tick = tick;
		*p_min_label = min_label;
	}
}

void draw_coordinate_system_axes(CoordinateSystem *coord_sys, int num_x_labels, int num_y_labels) {
	cairo_t *cr = coord_sys->screen->static_layer.cr;
	Vector2 min = coord_sys->min, max = coord_sys->max, origin = coord_sys->origin;
	// Set text color
	cairo_set_source_rgb(cr, 1, 1, 1);
	// Set font options
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12.0);
	int half_font_size = 5;

	// axes
	draw_stroke(cr, vec2(origin.x, 0), vec2(origin.x, origin.y));
	draw_stroke(cr, vec2(origin.x, origin.y), vec2(coord_sys->screen->width, origin.y));

	double y_label_tick, x_label_tick;
	double min_x_label, min_y_label;
	Datetime x_date_label_tick, y_date_label_tick;

	if(coord_sys->x_axis_type != CS_AXIS_DATE) {
		get_coordinate_system_axis_ticks(coord_sys->x_axis_type, num_x_labels, min.x, max.x, &x_label_tick, NULL, &min_x_label);
	} else {
		get_coordinate_system_axis_ticks(coord_sys->x_axis_type, num_x_labels, min.x, max.x, NULL, &x_date_label_tick, &min_x_label);
	}

	if(coord_sys->y_axis_type != CS_AXIS_DATE) {
		get_coordinate_system_axis_ticks(coord_sys->y_axis_type, num_y_labels, min.y, max.y, &y_label_tick, NULL, &min_y_label);
	} else {
		get_coordinate_system_axis_ticks(coord_sys->y_axis_type, num_y_labels, min.y, max.y, NULL, &y_date_label_tick, &min_y_label);
	}

	// gradients
	double m_y, m_x;
	m_x = (coord_sys->screen->width-origin.x)/(max.x - min.x);
	m_y = -origin.y/(max.y - min.y); // negative, because positive is down

	double y_label_x = origin.x-5,	x_label_y = origin.y + 20;

	// x-labels and x grid
	char string[32];
	for(int i = 0; i < num_x_labels; i++) {
		double label = coord_sys->x_axis_type != CS_AXIS_DATE ?
				min_x_label + i * x_label_tick :
				jd_change_date(min_x_label, i*x_date_label_tick.y, i*x_date_label_tick.m, i*x_date_label_tick.d, get_settings_datetime_type());
		if(label < min.x) continue;
		double x = (label-min.x)*m_x + origin.x;
		cairo_set_source_rgb(cr, 1, 1, 1);
		if(coord_sys->x_axis_type == CS_AXIS_NUMBER)
			sprintf(string, "%g", label);
		else if(coord_sys->x_axis_type == CS_AXIS_DURATION)
			sprintf(string, "%g", get_settings_datetime_type() == DATE_KERBAL ? label*4 : label);
		else if(coord_sys->x_axis_type == CS_AXIS_DATE)
			date_to_string(convert_JD_date(label, get_settings_datetime_type()), string, 0);
		draw_center_aligned_text(cr, x, x_label_y, string);
		cairo_set_source_rgb(cr, 0, 0, 0);
		draw_stroke(cr, vec2(x, origin.y), vec2(x, 0));
	}

	// y-labels and y grid
	for(int i = 0; i < num_y_labels; i++) {
		double label = coord_sys->y_axis_type != COORD_LABEL_DATE ?
					   min_y_label + i * y_label_tick :
					   jd_change_date(min_y_label, i*y_date_label_tick.y, i*y_date_label_tick.m, i*y_date_label_tick.d, get_settings_datetime_type());
		if(label < min.y) continue;
		double y = (label-min.y)*m_y + origin.y;
		cairo_set_source_rgb(cr, 1, 1, 1);
		if(coord_sys->y_axis_type == CS_AXIS_NUMBER)
			sprintf(string, "%g", label);
		else if(coord_sys->y_axis_type == CS_AXIS_DURATION)
			sprintf(string, "%g", get_settings_datetime_type() == DATE_KERBAL ? label*4 : label);
		else if(coord_sys->y_axis_type == CS_AXIS_DATE)
			date_to_string(convert_JD_date(label, get_settings_datetime_type()), string, 0);
		draw_right_aligned_text(cr, y_label_x, y+half_font_size, M_PI/4, string);
		cairo_set_source_rgb(cr, 0, 0, 0);
		draw_stroke(cr, vec2(origin.x, y), vec2(coord_sys->screen->width, y));
	}
}

void draw_coordinate_system_data_group_plot(CoordinateSystem *coord_sys, CSDataPointGroup *group) {
	cairo_t *cr = coord_sys->screen->static_layer.cr;

	double min_x = coord_sys->min.x, min_y = coord_sys->min.y;
	double max_x = coord_sys->max.x, max_y = coord_sys->max.y;
	Vector2 origin = coord_sys->origin;

	double dx = max_x-min_x;
	double dy = max_y-min_y;

	// gradients
	double m_x, m_y;
	m_x = (coord_sys->screen->width-origin.x)/dx;
	m_y = -origin.y/dy; // negative, because positive is down

	// data
	cairo_set_source_rgb(cr, 0, 0.8, 0.8);
	for(int i = 1; i < group->num_points; i++) {
		Vector2 point0 = vec2(
			origin.x + m_x*(group->points[i-1].x - min_x),
			origin.y + m_y * (group->points[i-1].y - min_y));
		Vector2 point1 = vec2(
			origin.x + m_x*(group->points[i  ].x - min_x),
			origin.y + m_y * (group->points[i  ].y - min_y));

		draw_stroke(cr, point0, point1);
	}
}


void draw_coordinate_system_data(CoordinateSystem *coord_sys) {
	clear_screen(coord_sys->screen);
	draw_coordinate_system_axes(coord_sys, 8, 10);
	for(int i = 0; i < coord_sys->num_point_groups; i++)
		draw_coordinate_system_data_group_plot(coord_sys, coord_sys->groups[i]);
	draw_screen(coord_sys->screen);
}

void draw_hover_position(CoordinateSystem *coord_sys, Vector2 mouse_pos) {
	cairo_t *cr = coord_sys->screen->dynamic_layer.cr;

	// Set font options
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 14.0);
	int half_font_size = 5;
	double dx = coord_sys->max.x-coord_sys->min.x;
	double dy = coord_sys->max.y-coord_sys->min.y;


	// gradients
	double m_x, m_y;
	m_x = (coord_sys->screen->width-coord_sys->origin.x)/dx;
	m_y = -coord_sys->origin.y/dy; // negative, because positive is down

	Vector2 data_loc = vec2(
	 (mouse_pos.x-coord_sys->origin.x)/m_x + coord_sys->min.x,
	 (mouse_pos.y-coord_sys->origin.y)/m_y + coord_sys->min.y);

	cairo_set_source_rgb(cr, 1, 0.2, 0.0);


	draw_stroke(cr,
		vec2(mouse_pos.x, 0),
		vec2(mouse_pos.x, coord_sys->origin.y));
	draw_stroke(cr,
		vec2(coord_sys->origin.x, mouse_pos.y),
		vec2(coord_sys->screen->width, mouse_pos.y));



	double y_label_x = coord_sys->origin.x-5,	x_label_y = coord_sys->origin.y + 20;

	char string[32];
	if(coord_sys->x_axis_type == CS_AXIS_NUMBER)
		sprintf(string, "%g", data_loc.x);
	else if(coord_sys->x_axis_type == CS_AXIS_DURATION)
		sprintf(string, "%g", get_settings_datetime_type() == DATE_KERBAL ? data_loc.x*4 : data_loc.x);
	else if(coord_sys->x_axis_type == CS_AXIS_DATE)
		date_to_string(convert_JD_date(data_loc.x, get_settings_datetime_type()), string, 0);
	draw_center_aligned_text(cr, mouse_pos.x, x_label_y, string);
	if(coord_sys->y_axis_type == CS_AXIS_NUMBER)
		sprintf(string, "%g", data_loc.y);
	else if(coord_sys->y_axis_type == CS_AXIS_DURATION)
		sprintf(string, "%g", get_settings_datetime_type() == DATE_KERBAL ? data_loc.y*4 : data_loc.y);
	else if(coord_sys->y_axis_type == CS_AXIS_DATE)
		date_to_string(convert_JD_date(data_loc.y, get_settings_datetime_type()), string, 0);
	draw_right_aligned_text(cr, y_label_x, mouse_pos.y+half_font_size, M_PI/4, string);

}