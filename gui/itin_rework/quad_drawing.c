#include "quad_drawing.h"

#include <gtk/gtk.h>
#include "gui/drawing.h"
#include "gui/gui_tools/coordinate_system_drawing.h"
#include <sys/time.h>
#include <math.h>


void draw_quad(Quad *quad, CoordinateSystem *coord_sys, bool filled, bool static_layer) {
	Vector2 coord_points[4];
	cairo_t *cr = static_layer ? coord_sys->screen->static_layer.cr : coord_sys->screen->dynamic_layer.cr;

	int cw[4] = {QUAD_NW, QUAD_NE, QUAD_SE, QUAD_SW};

	for(int i = 0; i < 4; i++) {
		coord_points[i] = to_coordinate_system_space(quad->corner[cw[i]]->pos, coord_sys);
	}

	if(filled) {
		cairo_move_to(cr, coord_points[0].x, coord_points[0].y);
		for(int i = 1; i < 4; i++) {
			cairo_line_to(cr, coord_points[i].x, coord_points[i].y);
		}
		cairo_close_path(cr);

		cairo_fill(cr);
	} else {
		for(int i = 0; i < 4; i++) {
			draw_stroke(cr, vec2(coord_points[i%4].x, coord_points[i%4].y), vec2(coord_points[(i+1)%4].x, coord_points[(i+1)%4].y));
		}
	}
}

void draw_quad_skeleton(Quad *root_quad, CoordinateSystem *coord_sys) {
	cairo_t *cr = coord_sys->screen->static_layer.cr;

	if(is_quad_flag(root_quad, QUAD_FLAG_IS_LEAF)) {
		cairo_set_source_rgb(cr, 1,1,1);
		draw_quad(root_quad, coord_sys, false, true);
	} else {
		for(int i = 0; i < 4; i++) {
			if(root_quad->subquads[i])
				draw_quad_skeleton(root_quad->subquads[i], coord_sys);
		}
	}
}



void draw_quad_debug(Quad *root_quad, CoordinateSystem *coord_sys) {
	cairo_t *cr = coord_sys->screen->static_layer.cr;

	if(is_quad_flag(root_quad, QUAD_FLAG_INACTIVE)) cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
	else if(is_quad_flag(root_quad, QUAD_FLAG_SPLIT)) cairo_set_source_rgb(cr, 1, 0.6, 0);
	else if(is_quad_flag(root_quad, QUAD_FLAG_ACC_ERR)) cairo_set_source_rgb(cr, 1, 0, 0);
	else cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);

	if(!is_quad_flag(root_quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			if(root_quad->subquads[i])
				draw_quad_debug(root_quad->subquads[i], coord_sys);
		}
	} else {
		draw_quad(root_quad, coord_sys, true, true);
	}
}



void draw_quad_interpolated_points(Quad *root_quad, CoordinateSystem *coord_sys) {
	cairo_t *cr = coord_sys->screen->static_layer.cr;
	int width = coord_sys->screen->width;
	int height = coord_sys->screen->height;


	for(int x = (int) coord_sys->origin.x+1; x < width; x++) {
		for(int y = 0; y < coord_sys->origin.y; y++) {
			Vector2 p = from_coordinate_system_space(vec2(x, y), coord_sys);
			Quad *quad = get_quad_at_position(root_quad, p);
			if(!quad) continue;
			double interpl_value = get_quad_interpolated_value(quad, p, coord_sys->groups[0]->quad_val_idx);
			interpl_value = (interpl_value-coord_sys->min.z)/(coord_sys->max.z-coord_sys->min.z);
			set_color_from_value(cr, interpl_value);
			cairo_rectangle(cr, x, y, 1, 1);
			cairo_fill(cr);
		}
	}
}

void draw_quad_checks(CoordinateSystem *coord_sys, Vector2 mouse_pos) {
	Vector2 pos = from_coordinate_system_space(mouse_pos, coord_sys);
	Quad *quad = get_quad_at_position(coord_sys->groups[0]->root_quad, pos);
	if(!quad) return;

	cairo_t *cr = coord_sys->screen->dynamic_layer.cr;
	cairo_set_source_rgb(cr, 0,0,1);
	draw_quad(quad, coord_sys, true, false);


	cairo_set_source_rgb(cr, 0.8,0.2,0);
	for(int i = 0; i < 8; i++) {
		if(quad->neighbours[i]) {
			draw_quad(quad->neighbours[i], coord_sys, true, false);
		}
	}
}

void draw_quad_value(CoordinateSystem *coord_sys, Vector2 mouse_pos) {
	Vector2 pos = from_coordinate_system_space(mouse_pos, coord_sys);
	Quad *quad = get_quad_at_position(coord_sys->groups[0]->root_quad, pos);
	if(!quad) return;
	if(quad->center->num_val < coord_sys->groups[0]->quad_val_idx) return;

	cairo_t *cr = coord_sys->screen->dynamic_layer.cr;
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 14.0);
	int font_size = 10;

	cairo_set_source_rgb(cr,  0.2, 0.2, 0.2);
	cairo_rectangle(cr, coord_sys->screen->width - font_size*12, 0, font_size*12, font_size*2);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 1, 0.2, 0.0);
	char string[32];
	sprintf(string, "%g", get_quad_interpolated_value(quad, pos, coord_sys->groups[0]->quad_val_idx));
	draw_right_aligned_text(cr, coord_sys->screen->width - font_size*0.5, font_size*1.5, 0, string);
}