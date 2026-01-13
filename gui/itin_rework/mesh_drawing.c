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

void draw_mesh_triangle(cairo_t *cr, MeshTriangle2 triangle, CoordinateSystem *coord_sys) {
	cairo_set_source_rgb(cr, 1,1,1);
	Vector2 coord_points[3];

	for(int i = 0; i < 3; i++) {
		coord_points[i].x = (triangle.points[i]->pos.x - coord_sys->min.x)/(coord_sys->max.x - coord_sys->min.x) *
			(coord_sys->screen->width - coord_sys->origin.x) + coord_sys->origin.x;
		coord_points[i].y = -(triangle.points[i]->pos.y - coord_sys->min.y)/(coord_sys->max.y - coord_sys->min.y) *
			coord_sys->origin.y + coord_sys->origin.y;
	}

	for(int i = 0; i < 3; i++) {
		draw_stroke(cr, vec2(coord_points[i%3].x, coord_points[i%3].y), vec2(coord_points[(i+1)%3].x, coord_points[(i+1)%3].y));
	}
}

void draw_mesh_skeleton(Mesh2 *mesh, CoordinateSystem *coord_sys) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	cairo_t *cr = coord_sys->screen->static_layer.cr;

	cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
	for(int i = 0; i < mesh->num_triangles; i++) {
		draw_mesh_triangle(cr, *mesh->triangles[i], coord_sys);
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
		double min_x, max_x, min_y, max_y;
		for(int j = 0; j < 3; j++) {
			tri2d.points[j]->pos.x = (mesh->triangles[i]->points[j]->pos.x - coord_sys->min.x)/(coord_sys->max.x - coord_sys->min.x) *
				(coord_sys->screen->width - coord_sys->origin.x) + coord_sys->origin.x;
			tri2d.points[j]->pos.y = -(mesh->triangles[i]->points[j]->pos.y - coord_sys->min.y)/(coord_sys->max.y - coord_sys->min.y) *
				coord_sys->origin.y + coord_sys->origin.y;
		}
		find_2dtriangle_minmax(tri2d, &min_x, &max_x, &min_y, &max_y);
		if(max_x < coord_sys->origin.x || min_x > width || max_y < 0 || min_y > coord_sys->origin.y) continue;
		for(int x = (int)min_x+1; x <= max_x; x+=step) {
			for(int y = (int)min_y+1; y <= max_y; y+=step) {
				Vector2 p = vec2(x, y);
				if(x > coord_sys->origin.x && x < width && y >= 0 && y < coord_sys->origin.y && is_inside_triangle(tri2d, p)) {
					Vector3 tri3[3];
					for(int idx = 0; idx < 3; idx++) {
						struct ItinStep *ptr = mesh->triangles[i]->points[idx]->data;
						double vinf = mag_vec3(subtract_vec3(ptr->v_dep, ptr->prev->v_body));
						double dv_dep = dv_circ(ptr->prev->body, 50e3+ptr->prev->body->radius, vinf);

						tri3[idx].x = tri2d.points[idx]->pos.x;
						tri3[idx].y = tri2d.points[idx]->pos.y;
						tri3[idx].z = (dv_dep-4000)/6000;
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