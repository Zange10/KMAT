#ifndef KMAT_COORDINATE_SYSTEM_H
#define KMAT_COORDINATE_SYSTEM_H

#include "screen.h"
#include "gui/itin_rework/mesh.h"

typedef struct CoordinateSystem CoordinateSystem;
typedef struct CSDataPointGroup CSDataPointGroup;
typedef struct CSDataPoint CSDataPoint;
typedef enum CSAxisLabelType CSAxisLabelType;
typedef enum CSDataPlotType CSDataPlotType;

enum CSAxisLabelType {
	CS_AXIS_NUMBER,
	CS_AXIS_DURATION,
	CS_AXIS_DATE
};

enum CSDataPlotType {
	CS_PLOT_TYPE_PLOT,
	CS_PLOT_TYPE_SCATTER,
	CS_PLOT_TYPE_PLOT_SCATTER,
	CS_PLOT_TYPE_MESH_INTERPOLATION,
	CS_PLOT_TYPE_MESH_SKELETON,
	CS_PLOT_TYPE_MESH_TRIANGLE_DEBUG,
	CS_PLOT_TYPE_MESH_BOXES,
};

struct CSDataPoint {
	double x,y;
	PixelColor color;
};

struct CSDataPointGroup {
	size_t num_points;
	CSDataPoint *points;
	Mesh2 *mesh;
	void (*free_mesh_data_func)(void *data);
	bool free_mesh_on_clear;
	CSDataPlotType plot_type;
};

struct CoordinateSystem {
	Screen *screen;
	Vector3 min, max;
	Vector2 origin;
	Vector2 mouse_hover_pos;
	bool show_hover_position;
	CSDataPointGroup **groups;
	size_t num_point_groups, point_group_cap;
	CSAxisLabelType x_axis_type, y_axis_type;
};

CoordinateSystem * new_coordinate_system(GtkWidget *drawing_area);

void plot_data2(CoordinateSystem *coord_sys, DataArray2 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data);
void scatter_data2(CoordinateSystem *coord_sys, DataArray2 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data);
void plot_scatter_data2(CoordinateSystem *coord_sys, DataArray2 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data);
void scatter_data3(CoordinateSystem *coord_sys, DataArray3 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data);
void attach_mesh_to_coordinate_system(CoordinateSystem *coord_sys, Mesh2 *mesh, CSDataPlotType plot_type, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool free_mesh_on_clear, void (*free_data_func)(void *data), bool free_prev_data);
size_t get_coordinate_system_total_number_of_points(CoordinateSystem *coord_sys);
Vector2 to_coordinate_system_space(Vector2 val, CoordinateSystem *coord_sys);
Vector2 from_coordinate_system_space(Vector2 pos, CoordinateSystem *coord_sys);

#endif //KMAT_COORDINATE_SYSTEM_H