#ifndef KMAT_MESH_DRAWING_H
#define KMAT_MESH_DRAWING_H

#include "mesh.h"
#include "geometrylib.h"
#include "gui/gui_tools/coordinate_system.h"

void draw_mesh_skeleton(Mesh2 *mesh, CoordinateSystem *coord_sys);
void draw_mesh_interpolated_points(Mesh2 *mesh, CoordinateSystem *coord_sys);
void draw_mesh_boxes(Mesh2 *mesh, CoordinateSystem *coord_sys);
void draw_triangle_checks(CoordinateSystem *coord_sys, Vector2 mouse_pos);


#endif //KMAT_MESH_DRAWING_H