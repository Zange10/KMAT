#ifndef KMAT_QUAD_DRAWING_H
#define KMAT_QUAD_DRAWING_H

#include "geometrylib.h"
#include "gui/gui_tools/coordinate_system.h"
#include "quad.h"

void draw_quad_skeleton(Quad *root_quad, CoordinateSystem *coord_sys);
void draw_quad_interpolated_points(Quad *root_quad, CoordinateSystem *coord_sys);
void draw_quad_checks(CoordinateSystem *coord_sys, Vector2 mouse_pos);
void draw_quad_debug(Quad *root_quad, CoordinateSystem *coord_sys);


#endif //KMAT_QUAD_DRAWING_H
