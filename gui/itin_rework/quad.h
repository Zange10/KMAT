#ifndef KMAT_QUAD_H
#define KMAT_QUAD_H

#include "mesh.h"

typedef struct Quad Quad;

typedef struct QuadErrorFunc {
	bool (*func)(Quad *quad, void*);
	void *params;
} QuadErrorFunc;

typedef struct QuadBoundsFunc {
	bool (*func)(Quad *quad, void*);
	void *params;
} QuadBoundsFunc;

typedef struct QuadPointFunc {
	MeshPoint2 * (*func)(double, double, void*);
	void *params;
} QuadPointFunc;

typedef enum QuadFlag {
	QUAD_FLAG_IS_LEAF = 1 << 0,
	QUAD_FLAG_SPLIT = 1 << 1,
	QUAD_FLAG_ACC_ERR = 1 << 2,
	QUAD_FLAG_INACTIVE = 1 << 3
} QuadFlag;

typedef enum {
	QUAD_N = 0,
	QUAD_E = 1,
	QUAD_W = 2,
	QUAD_S = 3
} QuadEdge;

typedef enum {
	QUAD_NW = 0,
	QUAD_NE = 1,
	QUAD_SW = 2,
	QUAD_SE = 3
} QuadQuadrant;

typedef enum {
	QUAD_NNW = 0,
	QUAD_NNE = 1,
	QUAD_NWW = 2,
	QUAD_SWW = 3,
	QUAD_NEE = 4,
	QUAD_SEE = 5,
	QUAD_SSW = 6,
	QUAD_SSE = 7
} QuadNeighbourLoc;

typedef u_int8_t quad_flags;

typedef struct Quad {
	Quad *parent;
	MeshPoint2 *corner[4];
	MeshPoint2 *center;
	union {
		Quad *neighbours[8];
		Quad *subquads[4];
	};
	quad_flags flags;
	int rf_level;	// refinement level
} Quad;

Quad * create_quad_from_four_points(Quad *parent, MeshPoint2 *p00, MeshPoint2 *p01, MeshPoint2 *p10, MeshPoint2 *p11, QuadPointFunc point_func);
Quad * get_root_quad(Quad *quad);

void set_quad_flag(Quad *quad, QuadFlag flag);
void remove_quad_flag(Quad *quad, QuadFlag flag);
bool is_quad_flag(Quad *quad, QuadFlag flag);

bool is_inside_quad(Quad *quad, Vector2 pos);
Quad * get_quad_at_position(Quad *root_quad, Vector2 pos);
double get_quad_interpolated_value(Quad *quad, Vector2 pos, int value_idx);
Vector3 get_quad_min_values(Quad *quad, int quad_val_idx);
Vector3 get_quad_max_values(Quad *quad, int quad_val_idx);
double get_quad_min_value(Quad *quad, int quad_val_idx);
double get_quad_max_value(Quad *quad, int quad_val_idx);
int get_num_quad_leaves(Quad *quad);
void print_quadtree(Quad *quad);
bool is_quad_crossed_by_line(Quad *quad, DataArray2 *line);
void find_line_crossed_quads(Quad *quad, DataArray2 *line, Quad ***quad_array, size_t *quad_array_size, size_t *quad_array_cap);

void remove_out_of_bounds_quads(Quad *quad, QuadBoundsFunc bounds_func);
int update_quad_error_flag(Quad *quad, int min_rf_level, int max_rf_level, QuadErrorFunc errfunc);
int split_quads_with_flag(Quad *quad, QuadPointFunc point_func);

int split_quad(Quad *quad, QuadPointFunc point_func);

void free_quad(Quad *quad, bool free_mesh_point);

#endif //KMAT_QUAD_H
