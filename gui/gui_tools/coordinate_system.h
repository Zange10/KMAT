#ifndef KMAT_COORDINATE_SYSTEM_H
#define KMAT_COORDINATE_SYSTEM_H

#include "screen.h"

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
	CS_PLOT_TYPE_SCATTER
};

struct CSDataPoint {
	double x,y;
	PixelColor color;
};

struct CSDataPointGroup {
	size_t num_points;
	CSDataPoint *points;
	CSDataPlotType plot_type;
};

struct CoordinateSystem {
	Screen *screen;
	Vector2 min, max;
	Vector2 origin;
	CSDataPointGroup **groups;
	size_t num_point_groups, point_group_cap;
	CSAxisLabelType x_axis_type, y_axis_type;
};

CoordinateSystem * new_coordinate_system(GtkWidget *drawing_area);

void plot_data2(CoordinateSystem *coord_sys, DataArray2 *data, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_coord_system);


#endif //KMAT_COORDINATE_SYSTEM_H