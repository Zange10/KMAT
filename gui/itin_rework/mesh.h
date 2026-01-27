#ifndef KMAT_MESH_H
#define KMAT_MESH_H

#include "geometrylib.h"


typedef struct MeshPoint2 MeshPoint2;
typedef struct MeshTriangle2 MeshTriangle2;
typedef struct MeshGrid2 MeshGrid2;
typedef struct MeshBox2 MeshBox2;
typedef struct Mesh2 Mesh2;
typedef enum MeshTriangleBoundaryCondition MeshTriangleBoundaryCondition;
typedef enum MeshTriangleFlag MeshTriangleFlag;

enum MeshTriangleBoundaryCondition {TRIANGLE_OUTSIDE_BOUNDARY, TRIANGLE_INSIDE_BOUNDARY, TRIANGLE_CROSSING_BOUNDARY};


enum MeshTriangleFlag {
	TRI_FLAG_INACTIVE = 1 << 0,
	TRI_FLAG_WANTS_REFINEMENT = 1 << 1,
	TRI_FLAG_ACC_ERR = 1 << 2,
};

typedef u_int8_t mesh_triangle_flags;

struct MeshPoint2 {
	Vector2 pos;
	double val;
	MeshTriangle2 **triangles;
	size_t num_triangles;
	size_t triangle_cap;
	void *data;
};

struct MeshTriangle2 {
	MeshPoint2 *points[3];
	MeshTriangle2 *adj_triangles[3];
	int rf_level;	// refinement level
	int target_rf_level;	// target refinement level
	mesh_triangle_flags flags;
};

struct MeshGrid2 {
	MeshPoint2 ***points;
	size_t num_cols;
	size_t *num_col_rows;
};

typedef enum {
	MESHBOX_SUBBOXES,
	MESHBOX_TRIANGLES
} MeshBoxType;

struct MeshBox2 {
	Vector2 min, max;
	MeshBox2 *parent;
	MeshBoxType type;

	union {
		struct {
			MeshBox2 **boxes;
			size_t num;
			size_t cap;
		} subboxes;

		struct {
			MeshTriangle2 **triangles;
			size_t num;
			size_t cap;
		} tri;
	};
};

struct Mesh2 {
	MeshTriangle2 **triangles;
	MeshPoint2 **points;
	MeshBox2 *mesh_box;
	size_t num_triangles;
	size_t triangle_cap;
	size_t num_points;
	size_t point_cap;
};

Mesh2 * new_mesh();
void set_mesh_tri_flag(MeshTriangle2 *triangle, MeshTriangleFlag flag);
void remove_mesh_tri_flag(MeshTriangle2 *triangle, MeshTriangleFlag flag);
bool is_mesh_tri_flag(MeshTriangle2 *triangle, MeshTriangleFlag flag);
bool triangle_is_edge(MeshTriangle2 *triangle);
bool is_triangle_bouding_box_inside_rectangle(MeshTriangle2 *triangle, Vector2 min, Vector2 max);
MeshTriangle2 * create_triangle_from_three_points_with_rf_level(MeshPoint2 *p0, MeshPoint2 *p1, MeshPoint2 *p2, int rf_level, int target_rf_level);
MeshTriangle2 * create_triangle_from_three_points(MeshPoint2 *p0, MeshPoint2 *p1, MeshPoint2 *p2);
void find_2dtriangle_minmax(MeshTriangle2 *triangle, Vector2 *min, Vector2 *max);
int is_inside_triangle(MeshTriangle2 *triangle, Vector2 p);
double get_triangle_interpolated_value(Vector3 p0, Vector3 p1, Vector3 p2, Vector2 p);
Vector2 get_triangle_centroid(MeshTriangle2 *triangle);
MeshTriangle2 * get_mesh_triangle_at_position(Mesh2 *mesh, Vector2 pos);
double get_mesh_interpolated_value(Mesh2 *mesh, Vector2 p);
void add_triangle_to_mesh(Mesh2 *mesh, MeshTriangle2 *triangle);
void add_point_to_mesh(Mesh2 *mesh, MeshPoint2 *point);
MeshGrid2 * create_mesh_grid(DataArray2 *pos, void **data);
Mesh2 * create_mesh_from_grid(MeshGrid2 *grid);
Mesh2 * create_mesh_from_grid_w_angled_guideline(MeshGrid2 *grid, double gradient);
Mesh2 * create_mesh_from_multiple_grids_w_angled_guideline(MeshGrid2 ***grid, int num_cols, int *num_cols_row, double gradient);
Mesh2 * create_mesh_from_grid_delaunay(MeshGrid2 *grid);
Mesh2 * combine_meshes(Mesh2 *mesh0, Mesh2 *mesh1);
void rebuild_mesh_boxes(Mesh2 *mesh);
void remove_triangle_id_from_mesh(Mesh2 *mesh, int tri_idx, bool remove_lone_points);
void remove_triangle_from_mesh(Mesh2 *mesh, MeshTriangle2 *triangle, bool remove_lone_points);
void update_mesh_minmax(Mesh2 *mesh);
void free_grid_keep_points(MeshGrid2 *grid);
void free_mesh(Mesh2 *mesh, void (*free_data_func)(void *data));

#endif //KMAT_MESH_H