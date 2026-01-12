#ifndef KMAT_COORDINATE_SYSTEM_H
#define KMAT_COORDINATE_SYSTEM_H

#include "screen.h"

typedef struct CoordinateSystem CoordinateSystem;
typedef struct CSDataPointGroup CSDataPointGroup;
typedef struct CSDataPoint CSDataPoint;
typedef enum CSAxisLabelType CSAxisLabelType;

enum CSAxisLabelType {
	CS_AXIS_NUMBER,
	CS_AXIS_DURATION,
	CS_AXIS_DATE
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

void plot_data(CoordinateSystem *coord_sys, DataArray2 *data);


#endif //KMAT_COORDINATE_SYSTEM_H