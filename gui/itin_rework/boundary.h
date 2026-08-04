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
Vector2 get_boundary_min(Boundary bdr);
Vector2 get_boundary_max(Boundary bdr);
void free_boundary(Boundary *bdr);
Boundary get_quad_mesh_value_boundary(Quad *quad, double val, int val_idx, int num_quad_points, bool enclose_higher);
int match_quads_to_boundary(Quad *quad, Boundary *soft_bdr, Boundary *hard_bdr, int min_bdr_rf_level, int max_rf_level, bool rm_hard_bdr_crossed);
Boundary combine_boundaries(Boundary bdr0, Boundary bdr1);
void connect_boundary_ends(Boundary *bdr);
void remove_boundary_end_connections(Boundary *bdr);
void draw_boundary_in_coordinate_system(CoordinateSystem *coord_sys, int group_id);

#endif //KMAT_BOUNDARY_H
