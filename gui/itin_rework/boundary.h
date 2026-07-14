#ifndef KMAT_BOUNDARY_H
#define KMAT_BOUNDARY_H

#include "geometrylib_datatool.h"
#include "gui/gui_tools/coordinate_system.h"
#include "quad.h"


typedef struct Boundary {
	DataArray2 **upper_bdrs;
	DataArray2 **lower_bdrs;
	size_t num;
	size_t cap;
} Boundary;



Boundary create_new_boundary();
void append_to_boundary(Boundary *bdr, DataArray2 *upper, DataArray2 *lower);
bool is_point_inside_boundary(Vector2 p, Boundary bdr);
bool is_line_crossing_boundary(Vector2 p0, Vector2 p1, Boundary bdr);
bool is_quad_crossed_by_boundary(Quad *quad, Boundary bdr);
bool is_quad_inside_boundary(Quad *quad, Boundary bdr);
void free_boundary(Boundary *bdr);
Boundary combine_boundaries(Boundary bdr0, Boundary bdr1);
void connect_boundary_ends(Boundary *bdr);
void remove_boundary_end_connections(Boundary *bdr);
void plot_boundary(CoordinateSystem *coord_sys, Boundary bdr, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data);
void scatter_boundary(CoordinateSystem *coord_sys, Boundary bdr, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data);
void plot_scatter_boundary(CoordinateSystem *coord_sys, Boundary bdr, CSAxisLabelType x_axis_type, CSAxisLabelType y_axis_type, bool clear_prev_data);

#endif //KMAT_BOUNDARY_H
