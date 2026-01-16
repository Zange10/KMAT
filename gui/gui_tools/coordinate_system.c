#include "coordinate_system.h"
#include "coordinate_system_drawing.h"
#include "gui/itin_rework/mesh_drawing.h"
#include <math.h>


void on_coordinate_system();
void on_coordinate_system_zoom(GtkWidget *widget, GdkEventScroll *event, CoordinateSystem *coord_sys);
void on_resize_coordinate_system(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys);
void on_coordinate_system_drag(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys);
void update_coordinate_system_hover_position(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys);
void enable_coordinate_system_show_hover_position(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys);
void disable_coordinate_system_show_hover_position(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys);

CoordinateSystem * new_coordinate_system(GtkWidget *drawing_area) {
	CoordinateSystem *new_coordinate_system = malloc(sizeof(CoordinateSystem));

	new_coordinate_system->groups = NULL;
	new_coordinate_system->num_point_groups = 0;
	new_coordinate_system->point_group_cap = 0;
	new_coordinate_system->min = vec3(0, 0, 0);
	new_coordinate_system->max = vec3(0, 0, 0);
	new_coordinate_system->show_hover_position = false;
	new_coordinate_system->x_axis_type = CS_AXIS_NUMBER;
	new_coordinate_system->y_axis_type = CS_AXIS_NUMBER;
	new_coordinate_system->screen = new_screen(drawing_area, NULL, &on_screen_button_press, &on_screen_button_release, NULL, NULL);
	new_coordinate_system->origin = vec2(60, new_coordinate_system->screen->height-30);
	set_screen_background_color(new_coordinate_system->screen, 0.15, 0.15, 0.15);

	gtk_widget_add_events(drawing_area, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	g_signal_connect(drawing_area, "size-allocate", G_CALLBACK(on_resize_coordinate_system), new_coordinate_system);
	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(on_coordinate_system_drag), new_coordinate_system);
	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(update_coordinate_system_hover_position), new_coordinate_system);
	g_signal_connect(drawing_area, "enter-notify-event", G_CALLBACK(enable_coordinate_system_show_hover_position), new_coordinate_system);
	g_signal_connect(drawing_area, "leave-notify-event", G_CALLBACK(disable_coordinate_system_show_hover_position), new_coordinate_system);
	g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(on_coordinate_system_zoom), new_coordinate_system);

	return new_coordinate_system;
}

CoordinateSystem * new_coordinate_system_for_mesh(GtkWidget *drawing_area) {
	CoordinateSystem *new_coordinate_system = malloc(sizeof(CoordinateSystem));

	new_coordinate_system->groups = NULL;
	new_coordinate_system->num_point_groups = 0;
	new_coordinate_system->point_group_cap = 0;
	new_coordinate_system->min = vec3(0, 0, 0);
	new_coordinate_system->max = vec3(0, 0, 0);
	new_coordinate_system->show_hover_position = false;
	new_coordinate_system->x_axis_type = CS_AXIS_NUMBER;
	new_coordinate_system->y_axis_type = CS_AXIS_NUMBER;
	new_coordinate_system->screen = new_screen(drawing_area, NULL, &on_screen_button_press, &on_screen_button_release, NULL, NULL);
	new_coordinate_system->origin = vec2(60, new_coordinate_system->screen->height-30);
	set_screen_background_color(new_coordinate_system->screen, 0.15, 0.15, 0.15);

	gtk_widget_add_events(drawing_area, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	g_signal_connect(drawing_area, "size-allocate", G_CALLBACK(on_resize_coordinate_system), new_coordinate_system);
	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(on_coordinate_system_drag), new_coordinate_system);
	g_signal_connect(drawing_area, "motion-notify-event", G_CALLBACK(update_coordinate_system_hover_position), new_coordinate_system);
	g_signal_connect(drawing_area, "enter-notify-event", G_CALLBACK(enable_coordinate_system_show_hover_position), new_coordinate_system);
	g_signal_connect(drawing_area, "leave-notify-event", G_CALLBACK(disable_coordinate_system_show_hover_position), new_coordinate_system);
	g_signal_connect(drawing_area, "scroll-event", G_CALLBACK(on_coordinate_system_zoom), new_coordinate_system);

	return new_coordinate_system;
}

void on_coordinate_system_drag(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys) {
	if(!coord_sys->screen->dragging) return;

	Vector2 mouse_diff = {
		event->x-coord_sys->screen->last_mouse_pos.x,
		event->y-coord_sys->screen->last_mouse_pos.y
	};

	double dx = coord_sys->max.x-coord_sys->min.x;
	double dy = coord_sys->max.y-coord_sys->min.y;

	// gradients
	double m_x, m_y;
	m_x = (coord_sys->screen->width-coord_sys->origin.x)/dx;
	m_y = -coord_sys->origin.y/dy; // negative, because positive is down

	double x_diff = mouse_diff.x/m_x;
	double y_diff = mouse_diff.y/m_y;


	coord_sys->max.x -= x_diff;
	coord_sys->min.x -= x_diff;
	coord_sys->max.y -= y_diff;
	coord_sys->min.y -= y_diff;

	coord_sys->screen->last_mouse_pos.x = event->x;
	coord_sys->screen->last_mouse_pos.y = event->y;

	draw_coordinate_system_data(coord_sys);
}

void update_coordinate_system_hover_position(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys) {
	clear_dynamic_screen_layer(coord_sys->screen);
	Vector2 mouse = {event->x, event->y};

	if(!coord_sys->show_hover_position || mouse.x < coord_sys->origin.x || mouse.y > coord_sys->origin.y) {
		draw_screen(coord_sys->screen);
		return;
	}

	if(coord_sys->groups[0]->plot_type >= CS_PLOT_TYPE_MESH_INTERPOLATION) {
		draw_triangle_checks(coord_sys, mouse);
	}

	draw_hover_position(coord_sys, mouse);

	draw_screen(coord_sys->screen);
}


void on_coordinate_system_zoom(GtkWidget *widget, GdkEventScroll *event, CoordinateSystem *coord_sys) {
	Vector2 mouse = {event->x, event->y};

	double dx = coord_sys->max.x-coord_sys->min.x;
	double dy = coord_sys->max.y-coord_sys->min.y;


	// gradients
	double m_x, m_y;
	m_x = (coord_sys->screen->width-coord_sys->origin.x)/dx;
	m_y = -coord_sys->origin.y/dy; // negative, because positive is down

	Vector2 data_loc = vec2(
	 (mouse.x-coord_sys->origin.x)/m_x + coord_sys->min.x,
	 (mouse.y-coord_sys->origin.y)/m_y + coord_sys->min.y);

	double x_ratio = (data_loc.x-coord_sys->min.x)/dx;
	double y_ratio = (data_loc.y-coord_sys->min.y)/dy;


	if(event->direction == GDK_SCROLL_UP) { dx /= 1.05; dy /= 1.05; }
	if(event->direction == GDK_SCROLL_DOWN) { dx *= 1.05; dy *= 1.05; }

	if(mouse.x >= coord_sys->origin.x) {
		coord_sys->min.x = data_loc.x - x_ratio*dx;
		coord_sys->max.x = coord_sys->min.x + dx;
	}
	if(mouse.y <= coord_sys->origin.y) {
		coord_sys->min.y = data_loc.y - y_ratio*dy;
		coord_sys->max.y = coord_sys->min.y + dy;
	}

	draw_coordinate_system_data(coord_sys);
}

void on_resize_coordinate_system(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys) {
	resize_screen(coord_sys->screen);
	coord_sys->origin = vec2(60, coord_sys->screen->height-30);
	draw_coordinate_system_data(coord_sys);
}

void enable_coordinate_system_show_hover_position(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys) {
	if(coord_sys->num_point_groups > 0)	coord_sys->show_hover_position = true;
}

void disable_coordinate_system_show_hover_position(GtkWidget *widget, GdkEventButton *event, CoordinateSystem *coord_sys) {
	coord_sys->show_hover_position = false;
	clear_dynamic_screen_layer(coord_sys->screen);
	draw_screen(coord_sys->screen);
}

void clear_coordinate_system(CoordinateSystem *coord_sys) {
	for(int i = 0; i < coord_sys->num_point_groups; i++) {
		if(coord_sys->groups[i]->plot_type == CS_PLOT_TYPE_PLOT ||
			coord_sys->groups[i]->plot_type == CS_PLOT_TYPE_SCATTER ||
			coord_sys->groups[i]->plot_type == CS_PLOT_TYPE_PLOT_SCATTER) {

			free(coord_sys->groups[i]->points);
			free(coord_sys->groups[i]);
		} else {
			if(coord_sys->groups[i]->free_mesh_on_clear) {
				free_mesh(coord_sys->groups[i]->mesh, coord_sys->groups[i]->free_mesh_data_func);
			}
		}
	}
	coord_sys->num_point_groups = 0;
	coord_sys->min = vec3(0, 0, 0);
	coord_sys->max = vec3(0, 0, 0);
	coord_sys->x_axis_type = CS_AXIS_NUMBER;
	coord_sys->y_axis_type = CS_AXIS_NUMBER;
}

void add_data2_to_coordinate_system(CoordinateSystem *coord_sys, DataArray2 *data_array, CSDataPlotType plot_type) {
	if(data_array2_size(data_array) == 0) return;

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
	new_group->mesh = NULL;

	Vector2 *data = data_array2_get_data(data_array);

	for(int i = 0; i < new_group->num_points; i++) {
		new_group->points[i].x = data[i].x;
		new_group->points[i].y = data[i].y;
		new_group->points[i].color = (PixelColor) {0, 0.8, 0.8};

		if(coord_sys->num_point_groups == 0 && i == 0) {
			coord_sys->min = vec3(data[i].x, data[i].y, 0);
			coord_sys->max = vec3(data[i].x, data[i].y, 0);
		}

		if(data[i].x > coord_sys->max.x) coord_sys->max.x = data[i].x;
		if(data[i].x < coord_sys->min.x) coord_sys->min.x = data[i].x;
		if(data[i].y > coord_sys->max.y) coord_sys->max.y = data[i].y;
		if(data[i].y < coord_sys->min.y) coord_sys->min.y = data[i].y;
	}

	coord_sys->groups[coord_sys->num_point_groups++] = new_group;
}

void add_data3_to_coordinate_system(CoordinateSystem *coord_sys, DataArray3 *data_array, CSDataPlotType plot_type) {
	if(data_array3_size(data_array) == 0) return;

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
	new_group->num_points = data_array3_size(data_array);
	new_group->points = malloc(sizeof(CSDataPoint) * new_group->num_points);
	new_group->mesh = NULL;

	Vector3 *data = data_array3_get_data(data_array);
	double min_z = data[0].z, max_z = data[0].z;
	for(int i = 1; i < new_group->num_points; i++) {
		if(data[i].z < min_z) min_z = data[i].z;
		if(data[i].z > max_z) max_z = data[i].z;
	}

	for(int i = 0; i < new_group->num_points; i++) {
		new_group->points[i].x = data[i].x;
		new_group->points[i].y = data[i].y;

		double color_bias = (data[i].z - min_z) / (max_z - min_z);
		double r = i == 0 ? 1 : color_bias;
		double g = i == 0 ? 0 : 1-color_bias;
		double b = i == 0 ? 0 : 4*pow(color_bias-0.5,2);
		new_group->points[i].color = (PixelColor) {r, g, b};

		if(coord_sys->num_point_groups == 0 && i == 0) {
			coord_sys->min = vec3(data[i].x, data[i].y, 0);
			coord_sys->max = vec3(data[i].x, data[i].y, 0);
		}

		if(data[i].x > coord_sys->max.x) coord_sys->max.x = data[i].x;
		if(data[i].x < coord_sys->min.x) coord_sys->min.x = data[i].x;
		if(data[i].y > coord_sys->max.y) coord_sys->max.y = data[i].y;
		if(data[i].y < coord_sys->min.y) coord_sys->min.y = data[i].y;
	}

	coord_sys->groups[coord_sys->num_point_groups++] = new_group;
}

void add_mesh_to_coordinate_system(CoordinateSystem *coord_sys, Mesh2 *mesh, CSDataPlotType plot_type, bool free_mesh_on_clear, void (*free_data_func)(void *data)) {
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
	new_group->num_points = 0;
	new_group->points = NULL;
	new_group->mesh = mesh;
	new_group->free_mesh_on_clear = free_mesh_on_clear;
	new_group->free_mesh_data_func = free_data_func;

	coord_sys->min = vec3(mesh->points[0]->pos.x, mesh->points[0]->pos.y, mesh->points[0]->val);
	coord_sys->max = vec3(mesh->points[0]->pos.x, mesh->points[0]->pos.y, mesh->points[0]->val);
	for(int i = 1; i < mesh->num_points; i++) {
		if(mesh->points[i]->pos.x < coord_sys->min.x) coord_sys->min.x = mesh->points[i]->pos.x;
		if(mesh->points[i]->pos.x > coord_sys->max.x) coord_sys->max.x = mesh->points[i]->pos.x;
		if(mesh->points[i]->pos.y < coord_sys->min.y) coord_sys->min.y = mesh->points[i]->pos.y;
		if(mesh->points[i]->pos.y > coord_sys->max.y) coord_sys->max.y = mesh->points[i]->pos.y;
		if(mesh->points[i]->val	  < coord_sys->min.z) coord_sys->min.z = mesh->points[i]->val;
		if(mesh->points[i]->val   > coord_sys->max.z) coord_sys->max.z = mesh->points[i]->val;
	}

	coord_sys->groups[coord_sys->num_point_groups++] = new_group;
}

void plot_data2(CoordinateSystem *coord_sys, DataArray2 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data) {
	if(clear_prev_data) clear_coordinate_system(coord_sys);
	add_data2_to_coordinate_system(coord_sys, data, CS_PLOT_TYPE_PLOT);
	coord_sys->x_axis_type = x_axis_type;
	coord_sys->y_axis_type = y_axis_type;
	draw_coordinate_system_data(coord_sys);
}

void scatter_data2(CoordinateSystem *coord_sys, DataArray2 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data) {
	if(clear_prev_data) clear_coordinate_system(coord_sys);
	add_data2_to_coordinate_system(coord_sys, data, CS_PLOT_TYPE_SCATTER);
	coord_sys->x_axis_type = x_axis_type;
	coord_sys->y_axis_type = y_axis_type;
	draw_coordinate_system_data(coord_sys);
}

void plot_scatter_data2(CoordinateSystem *coord_sys, DataArray2 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data) {
	if(clear_prev_data) clear_coordinate_system(coord_sys);
	add_data2_to_coordinate_system(coord_sys, data, CS_PLOT_TYPE_PLOT_SCATTER);
	coord_sys->x_axis_type = x_axis_type;
	coord_sys->y_axis_type = y_axis_type;
	draw_coordinate_system_data(coord_sys);
}

void scatter_data3(CoordinateSystem *coord_sys, DataArray3 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data) {
	if(clear_prev_data) clear_coordinate_system(coord_sys);
	add_data3_to_coordinate_system(coord_sys, data, CS_PLOT_TYPE_SCATTER);
	coord_sys->x_axis_type = x_axis_type;
	coord_sys->y_axis_type = y_axis_type;
	draw_coordinate_system_data(coord_sys);
}

void attach_mesh_to_coordinate_system(CoordinateSystem *coord_sys, Mesh2 *mesh, CSDataPlotType plot_type, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool free_mesh_on_clear, void (*free_data_func)(void *data), bool free_prev_data) {
	if(free_prev_data) clear_coordinate_system(coord_sys);
	add_mesh_to_coordinate_system(coord_sys, mesh, plot_type, free_mesh_on_clear, free_data_func);
	coord_sys->x_axis_type = x_axis_type;
	coord_sys->y_axis_type = y_axis_type;

	draw_coordinate_system_data(coord_sys);
}


size_t get_coordinate_system_total_number_of_points(CoordinateSystem *coord_sys) {
	size_t num_points = 0;
	for(int i = 0; i < coord_sys->num_point_groups; i++) {
		num_points+= coord_sys->groups[i]->num_points;
	}
	return num_points;
}

Vector2 to_coordinate_system_space(Vector2 val, CoordinateSystem *coord_sys) {
	Vector2 coord;
	coord.x = (val.x - coord_sys->min.x)/(coord_sys->max.x - coord_sys->min.x) *
				(coord_sys->screen->width - coord_sys->origin.x) + coord_sys->origin.x;
	coord.y = -(val.y - coord_sys->min.y)/(coord_sys->max.y - coord_sys->min.y) *
				coord_sys->origin.y + coord_sys->origin.y;
	return coord;
}

Vector2 from_coordinate_system_space(Vector2 pos, CoordinateSystem *coord_sys) {
	Vector2 val;
	val.x = (pos.x - coord_sys->origin.x)/(coord_sys->screen->width - coord_sys->origin.x) *
				(coord_sys->max.x - coord_sys->min.x) + coord_sys->min.x;
	val.y = -(pos.y-coord_sys->origin.y)/coord_sys->origin.y *
				(coord_sys->max.y - coord_sys->min.y) + coord_sys->min.y;
	return val;
}