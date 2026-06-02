#include "quad.h"


Quad * create_quad_from_four_points(Quad *parent, MeshPoint2 *p00, MeshPoint2 *p01, MeshPoint2 *p10, MeshPoint2 *p11) {
	Quad *quad = malloc(sizeof(Quad));
	quad->parent = parent;
	if(parent) quad->rf_level = parent->rf_level+1;
	else quad->rf_level = 0;
	quad->corner[QUAD_NW] = p00;
	quad->corner[QUAD_NE] = p01;
	quad->corner[QUAD_SW] = p10;
	quad->corner[QUAD_SE] = p11;
	quad->center = NULL;
	for(int i = 0; i < 8; i++) quad->neighbours[i] = NULL;
	quad->flags = 0;
	set_quad_flag(quad, QUAD_FLAG_IS_LEAF);
	return quad;
}

Quad * get_root_quad(Quad *quad) {
	if(!quad) return NULL;
	while(quad->parent) quad = quad->parent;
	return quad;
}

void set_quad_flag(Quad *quad, QuadFlag flag) { quad->flags |= flag; }
void remove_quad_flag(Quad *quad, QuadFlag flag) { quad->flags &= ~flag; }
bool is_quad_flag(Quad *quad, QuadFlag flag) { return (quad->flags & flag) != 0; }

bool is_inside_quad(Quad *quad, Vector2 pos) {
	if(!quad) return false;
	if(pos.x < quad->corner[QUAD_NW]->pos.x) return false;
	if(pos.x > quad->corner[QUAD_NE]->pos.x) return false;
	if(pos.y > quad->corner[QUAD_NW]->pos.y) return false;
	if(pos.y < quad->corner[QUAD_SW]->pos.y) return false;
	return true;
}

Quad * get_quad_at_position(Quad *root_quad, Vector2 pos) {
	if(!root_quad) return NULL;
	if(is_quad_flag(root_quad, QUAD_FLAG_IS_LEAF)) return root_quad;

	for(int i = 0; i < 4; i++) {
		if(is_inside_quad(root_quad->subquads[i], pos))
			return get_quad_at_position(root_quad->subquads[i], pos);
	}
	return NULL;
}

double get_quad_interpolated_value(Quad *quad, Vector2 pos, int value_idx) {
	Vector3 p00 = vec3(quad->corner[QUAD_NW]->pos.x, quad->corner[QUAD_NW]->pos.y, quad->corner[QUAD_NW]->val[value_idx]);
	Vector3 p01 = vec3(quad->corner[QUAD_NE]->pos.x, quad->corner[QUAD_NE]->pos.y, quad->corner[QUAD_NE]->val[value_idx]);
	Vector3 p10 = vec3(quad->corner[QUAD_SW]->pos.x, quad->corner[QUAD_SW]->pos.y, quad->corner[QUAD_SW]->val[value_idx]);
	Vector3 p11 = vec3(quad->corner[QUAD_SE]->pos.x, quad->corner[QUAD_SE]->pos.y, quad->corner[QUAD_SE]->val[value_idx]);

	double tx = (pos.x - p00.x) / (p01.x - p00.x);
	double ty = (pos.y - p00.y) / (p10.y - p00.y);

	return (1.0 - tx) * (1.0 - ty) * p00.z +
		   tx         * (1.0 - ty) * p01.z +
		   (1.0 - tx) * ty         * p10.z +
		   tx         * ty         * p11.z;
}

double get_quad_min_value(Quad *root_quad, int quad_val_idx) {
	return 0;
}

double get_quad_max_value(Quad *root_quad, int quad_val_idx) {
	return 0;
}

void set_quad_neighbours(Quad *center, Quad *tl, Quad *tr, Quad *lt, Quad *lb, Quad *rt, Quad *rb, Quad *bl, Quad *br) {
	center->neighbours[0] = tl;
	center->neighbours[1] = tr;
	center->neighbours[2] = lt;
	center->neighbours[3] = lb;
	center->neighbours[4] = rt;
	center->neighbours[5] = rb;
	center->neighbours[6] = bl;
	center->neighbours[7] = br;
}

void update_neighbours_after_split(Quad *subquads[4], Quad *neighbours[8]) {
	set_quad_neighbours(subquads[QUAD_NW],
		neighbours[QUAD_NNW], neighbours[QUAD_NNW],
		neighbours[QUAD_NWW], neighbours[QUAD_NWW],
		subquads[QUAD_NE], subquads[QUAD_NE],
		subquads[QUAD_SW], subquads[QUAD_SW]);

	set_quad_neighbours(subquads[QUAD_NE],
		neighbours[QUAD_NNE], neighbours[QUAD_NNE],
		subquads[QUAD_NW], subquads[QUAD_NW],
		neighbours[QUAD_NEE], neighbours[QUAD_NEE],
		subquads[QUAD_SE], subquads[QUAD_SE]);

	set_quad_neighbours(subquads[QUAD_SW],
		subquads[QUAD_NW], subquads[QUAD_NW],
		neighbours[QUAD_SWW], neighbours[QUAD_SWW],
		subquads[QUAD_SE], subquads[QUAD_SE],
		neighbours[QUAD_SSW], neighbours[QUAD_SSW]);

	set_quad_neighbours(subquads[QUAD_SE],
		subquads[QUAD_NE], subquads[QUAD_NE],
		subquads[QUAD_SW], subquads[QUAD_SW],
		neighbours[QUAD_SEE], neighbours[QUAD_SEE],
		neighbours[QUAD_SSE], neighbours[QUAD_SSE]);

	int rf_level = subquads[0]->rf_level;

	if(neighbours[QUAD_NNW]) {
		if(neighbours[QUAD_NNW]->rf_level < rf_level) {
			neighbours[QUAD_NNW]->neighbours[QUAD_SSW] = subquads[QUAD_NW];
			neighbours[QUAD_NNW]->neighbours[QUAD_SSE] = subquads[QUAD_NE];
		} else {
			neighbours[QUAD_NNW]->neighbours[QUAD_SSW] = subquads[QUAD_NW];
			neighbours[QUAD_NNW]->neighbours[QUAD_SSE] = subquads[QUAD_NW];
			if(neighbours[QUAD_NNE]) {
				neighbours[QUAD_NNE]->neighbours[QUAD_SSW] = subquads[QUAD_NE];
				neighbours[QUAD_NNE]->neighbours[QUAD_SSE] = subquads[QUAD_NE];
			}
		}
	}

	if(neighbours[QUAD_NWW]) {
		if(neighbours[QUAD_NWW]->rf_level < rf_level) {
			neighbours[QUAD_NWW]->neighbours[QUAD_NEE] = subquads[QUAD_NW];
			neighbours[QUAD_NWW]->neighbours[QUAD_SEE] = subquads[QUAD_SW];
		} else {
			neighbours[QUAD_NWW]->neighbours[QUAD_NEE] = subquads[QUAD_NW];
			neighbours[QUAD_NWW]->neighbours[QUAD_SEE] = subquads[QUAD_NW];
			if(neighbours[QUAD_SWW]) {
				neighbours[QUAD_SWW]->neighbours[QUAD_NEE] = subquads[QUAD_SW];
				neighbours[QUAD_SWW]->neighbours[QUAD_SEE] = subquads[QUAD_SW];
			}
		}
	}

	if(neighbours[QUAD_NEE]) {
		if(neighbours[QUAD_NEE]->rf_level < rf_level) {
			neighbours[QUAD_NEE]->neighbours[QUAD_NWW] = subquads[QUAD_NE];
			neighbours[QUAD_NEE]->neighbours[QUAD_SWW] = subquads[QUAD_SE];
		} else {
			neighbours[QUAD_NEE]->neighbours[QUAD_NWW] = subquads[QUAD_NE];
			neighbours[QUAD_NEE]->neighbours[QUAD_SWW] = subquads[QUAD_NE];
			if(neighbours[QUAD_SEE]) {
				neighbours[QUAD_SEE]->neighbours[QUAD_NWW] = subquads[QUAD_SE];
				neighbours[QUAD_SEE]->neighbours[QUAD_SWW] = subquads[QUAD_SE];
			}
		}
	}

	if(neighbours[QUAD_SSW]) {
		if(neighbours[QUAD_SSW]->rf_level < rf_level) {
			neighbours[QUAD_SSW]->neighbours[QUAD_NNW] = subquads[QUAD_SW];
			neighbours[QUAD_SSW]->neighbours[QUAD_NNE] = subquads[QUAD_SE];
		} else {
			neighbours[QUAD_SSW]->neighbours[QUAD_NNW] = subquads[QUAD_SW];
			neighbours[QUAD_SSW]->neighbours[QUAD_NNE] = subquads[QUAD_SW];
			if(neighbours[QUAD_SSE]) {
				neighbours[QUAD_SSE]->neighbours[QUAD_NNW] = subquads[QUAD_SE];
				neighbours[QUAD_SSE]->neighbours[QUAD_NNE] = subquads[QUAD_SE];
			}
		}
	}
}

void divide_quad(Quad *quad) {
	if(!quad) return;
	if(!is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) return;

	Quad *neighbours[8];
	bool changed = false;

	do {
		changed = false;
		for(int i = 0; i < 8; i++) {neighbours[i] = quad->neighbours[i];}
		for(int i = 0; i < 8; i++) {
			if(!neighbours[i]) continue;
			if(neighbours[i]->rf_level < quad->rf_level) {
				divide_quad(neighbours[i]);
				changed = true;
			}
		}
	} while(changed);

	MeshPoint2 *corners[4];
	MeshPoint2 *edges[4];

	if(!quad->center)
		quad->center = create_mesh_point(
			vec2(
				(quad->corner[QUAD_NW]->pos.x+quad->corner[QUAD_NE]->pos.x)/2.0,
				(quad->corner[QUAD_NW]->pos.y+quad->corner[QUAD_SW]->pos.y)/2.0
				),
			NULL, 0);

	edges[QUAD_N] = create_mesh_point(vec2(quad->center->pos.x, quad->corner[QUAD_NW]->pos.y), NULL, 0);
	edges[QUAD_W] = create_mesh_point(vec2(quad->corner[QUAD_NW]->pos.x, quad->center->pos.y), NULL, 0);
	edges[QUAD_E] = create_mesh_point(vec2(quad->corner[QUAD_NE]->pos.x, quad->center->pos.y), NULL, 0);
	edges[QUAD_S] = create_mesh_point(vec2(quad->center->pos.x, quad->corner[QUAD_SW]->pos.y), NULL, 0);


	for(int i = 0; i < 4; i++) {
		switch(i) {
			case 0:
				corners[QUAD_NW] = quad->corner[QUAD_NW];
				corners[QUAD_NE] = edges[QUAD_N];
				corners[QUAD_SW] = edges[QUAD_W];
				corners[QUAD_SE] = quad->center;
				break;
			case 1:
				corners[QUAD_NW] = edges[QUAD_N];
				corners[QUAD_NE] = quad->corner[QUAD_NE];
				corners[QUAD_SW] = quad->center;
				corners[QUAD_SE] = edges[QUAD_E];
				break;
			case 2:
				corners[QUAD_NW] = edges[QUAD_W];
				corners[QUAD_NE] = quad->center;
				corners[QUAD_SW] = quad->corner[QUAD_SW];
				corners[QUAD_SE] = edges[QUAD_S];
				break;
			case 3:
				corners[QUAD_NW] = quad->center;
				corners[QUAD_NE] = edges[QUAD_E];
				corners[QUAD_SW] = edges[QUAD_S];
				corners[QUAD_SE] = quad->corner[QUAD_SE];
				break;
			default:
				corners[QUAD_NW] = NULL;
				corners[QUAD_NE] = NULL;
				corners[QUAD_SW] = NULL;
				corners[QUAD_SE] = NULL;
		}

		quad->subquads[i] = create_quad_from_four_points(quad, corners[QUAD_NW], corners[QUAD_NE], corners[QUAD_SW], corners[QUAD_SE]);
	}
	update_neighbours_after_split(quad->subquads, neighbours);
	quad->flags = 0;
}


void free_quad(Quad *quad, bool free_mesh_point) {
	if(!is_quad_flag(quad, QUAD_FLAG_IS_LEAF)) {
		for(int i = 0; i < 4; i++) {
			if(quad->subquads[i]) {
				free_quad(quad->subquads[i], free_mesh_point);
			}
		}
	} else {
		for(int i = 0; i < 8; i++) {
			if(!quad->neighbours[i]) continue;
			for(int j = 0; j < 8; j++) {
				if(!quad->neighbours[i]->neighbours[j]) continue;
				if(quad->neighbours[i]->neighbours[j] == quad) quad->neighbours[i]->neighbours[j] = NULL;
			}
		}



		if(free_mesh_point) {
			// free mesh point if lonely
		}
	}

	if(quad->parent) {
		for(int i = 0; i < 4; i++) {
			if(quad->parent->subquads[i] == quad) quad->parent->subquads[i] = NULL;
		}
	}
}