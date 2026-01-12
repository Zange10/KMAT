#include "coordinate_system.h"
#include "coordinate_system_drawing.h"


void on_coordinate_system();
void on_coordinate_system_zoom();
void on_resize_coordinate_system(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys);

CoordinateSystem * new_coordinate_system(GtkWidget *drawing_area) {
	CoordinateSystem *new_coordinate_system = malloc(sizeof(CoordinateSystem));

	new_coordinate_system->groups = NULL;
	new_coordinate_system->num_point_groups = 0;
	new_coordinate_system->point_group_cap = 0;
	new_coordinate_system->min = vec2(0, 0);
	new_coordinate_system->max = vec2(0, 0);
	new_coordinate_system->x_axis_type = CS_AXIS_NUMBER;
	new_coordinate_system->y_axis_type = CS_AXIS_NUMBER;
	new_coordinate_system->screen = new_screen(drawing_area, NULL, NULL, NULL, NULL, NULL);
	new_coordinate_system->origin = vec2(60, new_coordinate_system->screen->height-30);
	set_screen_background_color(new_coordinate_system->screen, 0.15, 0.15, 0.15);

	g_signal_connect(drawing_area, "size-allocate", G_CALLBACK(on_resize_coordinate_system), new_coordinate_system);
	// if(button_press_func != NULL) 	g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(button_press_func), new_screen);
	// if(button_release_func != NULL)	g_signal_connect(drawing_area, "button-release-event", G_CALLBACK(button_release_func), new_screen);
	// if(mouse_motion_func != NULL) 	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(mouse_motion_func), new_screen);
	// if(scroll_func != NULL) 		g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(scroll_func), new_screen);

	return new_coordinate_system;
}

void on_resize_coordinate_system(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys) {
	resize_screen(coord_sys->screen);
	coord_sys->origin = vec2(60, coord_sys->screen->height-30);
	draw_coordinate_system_data(coord_sys);
}

void clear_coordinate_system(CoordinateSystem *coord_sys) {
	for(int i = 0; i < coord_sys->num_point_groups; i++) {
		free(coord_sys->groups[i]->points);
		free(coord_sys->groups[i]);
	}
	coord_sys->num_point_groups = 0;
}

void add_data_to_coordinate_system(CoordinateSystem *coord_sys, DataArray2 *data_array, CSDataPlotType plot_type) {
	if(coord_sys->num_point_groups+1 >= coord_sys->point_group_cap) {
		if(coord_sys->point_group_cap == 0) {
			coord_sys->point_group_cap = 1;
			coord_sys->groups = malloc(coord_sys->point_group_cap * sizeof(CSDataPointGroup *));
		} else {
			coord_sys->point_group_cap *= 2;
			CSDataPointGroup **temp = realloc(coord_sys->groups, coord_sys->point_group_cap * sizeof(CSDataPointGroup *));
			if(temp) coord_sys->groups = temp;
		}
	}

	CSDataPointGroup *new_group = malloc(sizeof(CSDataPointGroup));
	new_group->plot_type = plot_type;
	new_group->num_points = data_array2_size(data_array);
	new_group->points = malloc(sizeof(CSDataPoint) * new_group->num_points);

	Vector2 *data = data_array2_get_data(data_array);

	for(int i = 0; i < new_group->num_points; i++) {
		new_group->points[i].x = data[i].x;
		new_group->points[i].y = data[i].y;
		new_group->points[i].color = (PixelColor) {1, 0.4, 0.1};

		if(coord_sys->num_point_groups == 0 && i == 0) {
			coord_sys->min = vec2(data[i].x, data[i].y);
			coord_sys->max = vec2(data[i].x, data[i].y);
		}

		if(data[i].x > coord_sys->max.x) coord_sys->max.x = data[i].x;
		if(data[i].x < coord_sys->min.x) coord_sys->min.x = data[i].x;
		if(data[i].y > coord_sys->max.y) coord_sys->max.y = data[i].y;
		if(data[i].y < coord_sys->min.y) coord_sys->min.y = data[i].y;
	}

	coord_sys->groups[coord_sys->num_point_groups++] = new_group;
}

void plot_data2(CoordinateSystem *coord_sys, DataArray2 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_coord_system) {
	if(clear_coord_system) clear_coordinate_system(coord_sys);
	add_data_to_coordinate_system(coord_sys, data, CS_PLOT_TYPE_PLOT);
	coord_sys->x_axis_type = x_axis_type;
	coord_sys->y_axis_type = y_axis_type;
	draw_coordinate_system_data(coord_sys);
}