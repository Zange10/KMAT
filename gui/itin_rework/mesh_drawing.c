#include "mesh_drawing.h"

#include <gtk/gtk.h>
#include "gui/drawing.h"
#include "gui/gui_tools/coordinate_system_drawing.h"
#include <sys/time.h>
#include <math.h>


void resize_pcmesh_to_fit(Mesh2 mesh, double max_x, double max_y) {
	Vector2 max = mesh.points[0]-> pos;
	Vector2 min = mesh.points[0]-> pos;

	for(int i = 0; i < mesh.num_points; i++) {
		if(mesh.points[i]-> pos.x > max.x) max.x = mesh.points[i]-> pos.x;
		if(mesh.points[i]-> pos.x < min.x) min.x = mesh.points[i]-> pos.x;
		if(mesh.points[i]-> pos.y > max.y) max.y = mesh.points[i]-> pos.y;
		if(mesh.points[i]-> pos.y < min.y) min.y = mesh.points[i]-> pos.y;
	}

	max.x += 0.1*(max.x-min.x);
	min.x -= 0.1*(max.x-min.x);
	max.y += 0.1*(max.y-min.y);
	min.y -= 0.1*(max.y-min.y);

	Vector2 gradient = {
		max_x/(max.x-min.x),
		max_y/(max.y-min.y)
	};


	for(int i = 0; i < mesh.num_points; i++) {
		mesh.points[i]-> pos.x -= min.x;
		mesh.points[i]-> pos.x *= gradient.x;
		mesh.points[i]-> pos.y -= min.y;
		mesh.points[i]-> pos.y *= gradient.y;
		mesh.points[i]-> pos.y  = max_y-mesh.points[i]-> pos.y;
	}
}

void draw_mesh_triangle(MeshTriangle2 *triangle, CoordinateSystem *coord_sys, bool filled, bool static_layer) {
	Vector2 coord_points[3];
	cairo_t *cr = static_layer ? coord_sys->screen->static_layer.cr : coord_sys->screen->dynamic_layer.cr;

	for(int i = 0; i < 3; i++) {
		coord_points[i] = to_coordinate_system_space(triangle->points[i]->pos, coord_sys);
	}

	if(filled) {
		cairo_move_to(cr, coord_points[0].x, coord_points[0].y);
		cairo_line_to(cr, coord_points[1].x, coord_points[1].y);
		cairo_line_to(cr, coord_points[2].x, coord_points[2].y);
		cairo_close_path(cr);

		cairo_fill(cr);
	} else {
		for(int i = 0; i < 3; i++) {
			draw_stroke(cr, vec2(coord_points[i%3].x, coord_points[i%3].y), vec2(coord_points[(i+1)%3].x, coord_points[(i+1)%3].y));
		}
	}
}

void draw_mesh_skeleton(Mesh2 *mesh, CoordinateSystem *coord_sys) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	cairo_t *cr = coord_sys->screen->static_layer.cr;

	cairo_set_source_rgb(cr, 1,1,1);
	for(int i = 0; i < mesh->num_triangles; i++) {
		draw_mesh_triangle(mesh->triangles[i], coord_sys, false, true);
	}

	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Mesh Drawing: %.6f seconds\n", duration);
	printf("Triangles drawn: %zu\n", mesh->num_triangles);
}



void draw_triangle_debug(cairo_t *cr, Mesh2 *mesh) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for(int i = 0; i < mesh->num_triangles; i++) {
		if(triangle_is_edge(mesh->triangles[i])) cairo_set_source_rgb(cr, 0, 0.5, 1);
		else cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);

		cairo_move_to(cr, mesh->triangles[i]->points[0]->pos.x, mesh->triangles[i]->points[0]->pos.y);
		cairo_line_to(cr, mesh->triangles[i]->points[1]->pos.x, mesh->triangles[i]->points[1]->pos.y);
		cairo_line_to(cr, mesh->triangles[i]->points[2]->pos.x, mesh->triangles[i]->points[2]->pos.y);
		cairo_close_path(cr);

		cairo_fill(cr);
	}



	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Interpolation Drawing: %.6f seconds\n", duration);

	printf("Triangles drawn: %zu\n", mesh->num_triangles);
}



void set_color_from_value(cairo_t *cr, double value) {
	double r = value;
	double g = 1-value;
	double b = 4*pow(value-0.5,2);
	cairo_set_source_rgb(cr, r,g,b);
}

void draw_mesh_interpolated_points(Mesh2 *mesh, CoordinateSystem *coord_sys) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	cairo_t *cr = coord_sys->screen->static_layer.cr;
	int width = coord_sys->screen->width;
	int step = 1;
	int pixel_counter = 0;

	MeshTriangle2 tri2d;
	for(int i = 0; i < 3; i++) tri2d.points[i] = malloc(sizeof(MeshPoint2));

	for(int i = 0; i < mesh->num_triangles; i++) {
		Vector2 min, max;
		for(int j = 0; j < 3; j++) {
			tri2d.points[j]->pos.x = (mesh->triangles[i]->points[j]->pos.x - coord_sys->min.x)/(coord_sys->max.x - coord_sys->min.x) *
				(coord_sys->screen->width - coord_sys->origin.x) + coord_sys->origin.x;
			tri2d.points[j]->pos.y = -(mesh->triangles[i]->points[j]->pos.y - coord_sys->min.y)/(coord_sys->max.y - coord_sys->min.y) *
				coord_sys->origin.y + coord_sys->origin.y;
		}
		find_2dtriangle_minmax(&tri2d, &min, &max);
		if(max.x < coord_sys->origin.x || min.x > width || max.y < 0 || min.y > coord_sys->origin.y) continue;
		for(int x = (int)min.x+1; x <= max.x; x+=step) {
			for(int y = (int)min.y+1; y <= max.y; y+=step) {
				Vector2 p = vec2(x, y);
				if(x > coord_sys->origin.x && x < width && y >= 0 && y < coord_sys->origin.y && is_inside_triangle(&tri2d, p)) {
					Vector3 tri3[3];
					for(int idx = 0; idx < 3; idx++) {
						tri3[idx].x = tri2d.points[idx]->pos.x;
						tri3[idx].y = tri2d.points[idx]->pos.y;
						tri3[idx].z = (mesh->triangles[i]->points[idx]->val - coord_sys->min.z)/(coord_sys->max.z - coord_sys->min.z);
					}
					double interpl_value = get_triangle_interpolated_value(tri3[0], tri3[1], tri3[2], p);

					set_color_from_value(cr, interpl_value);
					cairo_rectangle(cr, x, y, 1, 1);
					cairo_fill(cr);
					pixel_counter++;
				}
			}
		}
	}

	for(int i = 0; i < 3; i++) free(tri2d.points[i]);

	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Interpolated Mesh Drawing: %.6f seconds\n", duration);
	printf("Pixels drawn: %d\n", pixel_counter);
}

void draw_mesh_box(MeshBox2 *box, CoordinateSystem *coord_sys, int level, bool static_layer) {
	cairo_t *cr = static_layer ? coord_sys->screen->static_layer.cr : coord_sys->screen->dynamic_layer.cr;
	switch(level) {
		case 0:
			cairo_set_source_rgb(cr, 1,1,1);
			break;
		case 1:
			cairo_set_source_rgb(cr, 0,0,1);
			break;
		case 2:
			cairo_set_source_rgb(cr, 0,1,0);
			break;
		case 3:
			cairo_set_source_rgb(cr, 1,1,0);
			break;
		case 4:
			cairo_set_source_rgb(cr, 1,0,1);
			break;
		default:
			cairo_set_source_rgb(cr, 1,1,1);
			break;
	}

	Vector2 min = to_coordinate_system_space(box->min, coord_sys);
	Vector2 max = to_coordinate_system_space(box->max, coord_sys);

	draw_stroke(cr, vec2(min.x, min.y), vec2(max.x, min.y));
	draw_stroke(cr, vec2(max.x, min.y), vec2(max.x, max.y));
	draw_stroke(cr, vec2(max.x, max.y), vec2(min.x, max.y));
	draw_stroke(cr, vec2(min.x, max.y), vec2(min.x, min.y));
}

int draw_mesh_box_and_subboxes(MeshBox2 *box, int level, CoordinateSystem *coord_sys) {
	int num_boxes_drawn = 0;

	if(box->type == MESHBOX_SUBBOXES) {
		for(int i = 0; i < box->subboxes.num; i++) {
			num_boxes_drawn += draw_mesh_box_and_subboxes(box->subboxes.boxes[i], level+1, coord_sys);
		}
	}

	draw_mesh_box(box, coord_sys, level, true);
	num_boxes_drawn++;

	return num_boxes_drawn;
}

void draw_mesh_boxes(Mesh2 *mesh, CoordinateSystem *coord_sys) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	int num_boxes_drawn = draw_mesh_box_and_subboxes(mesh->mesh_box, 0, coord_sys);

	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Mesh Boxes Drawing: %.6f seconds\n", duration);
	printf("Boxes drawn: %d\n", num_boxes_drawn);
}

void draw_mesh_triangle_and_boxes_for_position_lookup(MeshBox2 *box, Vector2 pos, int level, CoordinateSystem *coord_sys) {
	if(!box) return;
	cairo_t *cr = coord_sys->screen->dynamic_layer.cr;

	if(box->type == MESHBOX_SUBBOXES) {
		for(int i = 0; i < box->subboxes.num; i++) {
			if(pos.x >= box->subboxes.boxes[i]->min.x && pos.x <= box->subboxes.boxes[i]->max.x &&
				pos.y >= box->subboxes.boxes[i]->min.y && pos.y <= box->subboxes.boxes[i]->max.y) {
				draw_mesh_triangle_and_boxes_for_position_lookup(box->subboxes.boxes[i], pos, level+1, coord_sys);
				break;
			}
		}
	} else if(box->type == MESHBOX_TRIANGLES) {
		for(int i = 0; i < box->tri.num; i++) {
			if(is_inside_triangle(box->tri.triangles[i], pos)) cairo_set_source_rgb(cr, 0,1,0);
			else cairo_set_source_rgb(cr, 0,0,1);
			draw_mesh_triangle(box->tri.triangles[i], coord_sys, true, false);
		}
	}
	draw_mesh_box(box, coord_sys, level, false);
}

void draw_triangle_checks(CoordinateSystem *coord_sys, Vector2 mouse_pos) {
	Vector2 val = from_coordinate_system_space(mouse_pos, coord_sys);
	draw_mesh_triangle_and_boxes_for_position_lookup(coord_sys->groups[0]->mesh->mesh_box, val, 0, coord_sys);
}