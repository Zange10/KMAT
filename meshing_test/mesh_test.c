#include "mesh_test.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <math.h>
#include <sys/time.h>
#include "tools/analytic_geometry.h"
#include "gui/drawing.h"
#include "mesh_creator.h"
#include "tools/file_io.h"


GObject *mesh_drawing_area;
PcMesh mesh;
PcMeshGrid mesh_grid;
PcMeshGroups pc_point_groups;

double angle = 0;

uint32_t *pixel_data;
cairo_surface_t *image_surface;


typedef struct {
	struct Vector2D points[3];
} MeshTriangle2D;

MeshTriangle2D convert_mesh_triangle_3d_to_2d(PcMeshTriangle tri3d) {
	MeshTriangle2D tri2d;
	for(int i = 0; i < 3; i++) tri2d.points[i] = vec2D(tri3d.points[i]->data.x, tri3d.points[i]->data.y);
	return tri2d;
}

struct Vector2D subtract_vec2d(struct Vector2D v0, struct Vector2D v1) {
	return vec2D(v0.x-v1.x, v0.y-v1.y);
}

int is_inside_triangle(MeshTriangle2D triangle, struct Vector2D p) {
	struct Vector2D a = triangle.points[0];
	struct Vector2D b = triangle.points[1];
	struct Vector2D c = triangle.points[2];
	struct Vector2D v0 = subtract_vec2d(c,a);
	struct Vector2D v1 = subtract_vec2d(b,a);
	struct Vector2D v2 = subtract_vec2d(p,a);

	double d00 = dot_product2d(v0, v0);
	double d01 = dot_product2d(v0, v1);
	double d11 = dot_product2d(v1, v1);
	double d20 = dot_product2d(v2, v0);
	double d21 = dot_product2d(v2, v1);

	double denom = d00*d11 - d01*d01;

	double u = (d11*d20 - d01*d21) / denom;
	double v = (d00*d21 - d01*d20) / denom;

	return (u >= 0 && v >= 0 && u+v <= 1+1e-9);
}

void find_2dtriangle_minmax(MeshTriangle2D triangle, double *min_x, double *max_x, double *min_y, double *max_y) {
	*min_x = triangle.points[0].x;
	*max_x = triangle.points[0].x;
	*min_y = triangle.points[0].y;
	*max_y = triangle.points[0].y;

	for(int i = 1; i < 3; i++) {
		if(triangle.points[i].x < *min_x) *min_x = triangle.points[i].x;
		if(triangle.points[i].x > *max_x) *max_x = triangle.points[i].x;
		if(triangle.points[i].y < *min_y) *min_y = triangle.points[i].y;
		if(triangle.points[i].y > *max_y) *max_y = triangle.points[i].y;
	}
}

PcMeshTriangle * find_triangle_for_point(struct Vector2D p) {
	for(int i = 0; i < mesh.num_triangles; i++) {
		if(is_inside_triangle(convert_mesh_triangle_3d_to_2d(*mesh.triangles[i]), p)) {
			return &mesh.triangles[i];
		}
	}
	return NULL;
}

double get_triangle_interpolated_value(PcMeshTriangle triangle, struct Vector2D p) {
	MeshTriangle2D tri2d = convert_mesh_triangle_3d_to_2d(triangle);
	struct Vector2D a = tri2d.points[0];
	struct Vector2D b = tri2d.points[1];
	struct Vector2D c = tri2d.points[2];
	struct Vector2D v0 = subtract_vec2d(c,a);
	struct Vector2D v1 = subtract_vec2d(b,a);
	struct Vector2D v2 = subtract_vec2d(p,a);

	double d00 = dot_product2d(v0, v0);
	double d01 = dot_product2d(v0, v1);
	double d11 = dot_product2d(v1, v1);
	double d20 = dot_product2d(v2, v0);
	double d21 = dot_product2d(v2, v1);

	double denom = d00*d11 - d01*d01;

	double v = (d00*d21 - d01*d20) / denom;
	double w = (d11*d20 - d01*d21) / denom;
	double u = 1-v-w;

	return u*triangle.points[0]->data.z + v*triangle.points[1]->data.z + w*triangle.points[2]->data.z;
}


void set_color_from_value(cairo_t *cr, double value) {
	double r = value;
	double g = 1-value;
	double b = 4*pow(value-0.5,2);
	cairo_set_source_rgb(cr, r,g,b);
}

void get_color_from_value(double value, uint32_t *color) {
	// Normalize to [0, 1]
	if (value < 0) value = 0;
	if (value > 1) value = 1;

	uint8_t r = (uint8_t)(255 * value);
	uint8_t g = (uint8_t)(255 * (1.0 - value));
	uint8_t b = (uint8_t)(255 * 4.0 * (value - 0.5) * (value - 0.5));

	*color = (r << 16) | (g << 8) | b;
}


void activate_mesh_test(GtkApplication *app, gpointer user_data);

void draw_mesh_triangle(cairo_t *cr, PcMeshTriangle triangle) {
	cairo_set_source_rgb(cr, 1,1,1);
	for(int i = 0; i < 3; i++) {
		draw_stroke(cr, vec2D(triangle.points[i]->data.x, triangle.points[i]->data.y), vec2D(triangle.points[(i+1)%3]->data.x, triangle.points[(i+1)%3]->data.y));
	}
}

void draw_mesh(cairo_t *cr) {
	struct timeval start, end;
	gettimeofday(&start, NULL);
	cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
	for(int i = 0; i < mesh.num_triangles; i++) {
		draw_mesh_triangle(cr, *mesh.triangles[i]);
	}

	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Mesh Drawing: %.6f seconds\n", duration);
	printf("Triangles drawn: %zu\n", mesh.num_triangles);
}

void draw_points(cairo_t *cr) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for(int c = 0; c < mesh.num_points; c++) {
		PcMeshPoint point = *mesh.points[c];
//		set_color_from_value(cr, triangle.points[i]->data.z);
		cairo_set_source_rgb(cr, point.group->color[0], point.group->color[1], point.group->color[2]);
//		if(point.is_edge) cairo_set_source_rgb(cr, 0, 0.5, 1);
//		if(point.is_artificial) cairo_set_source_rgb(cr, 1, 0.2, 0);
		cairo_arc(cr, point.data.x, point.data.y, 2, 0, 2*M_PI);
		cairo_fill(cr);
	}

	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Point Drawing: %.6f seconds\n", duration);
}

void draw_mesh_interpolated_points(cairo_t *cr, int width, int height) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	int step = 1;

	for(int i = 0; i < mesh.num_triangles; i++) {
		double min_x, max_x, min_y, max_y;
		MeshTriangle2D tri2d = convert_mesh_triangle_3d_to_2d(*mesh.triangles[i]);
		find_2dtriangle_minmax(tri2d, &min_x, &max_x, &min_y, &max_y);
		if(max_x < 0 || min_x > width || max_y < 0 || min_y > height) continue;
		for(int x = (int)min_x+1; x <= max_x; x+=step) {
			for(int y = (int)min_y+1; y <= max_y; y+=step) {
				struct Vector2D p = vec2D(x, y);
				if(x >= 0 && x < width && y >= 0 && y < height && is_inside_triangle(tri2d, p)) {
					get_color_from_value(get_triangle_interpolated_value(*mesh.triangles[i], p), &pixel_data[y*width+x]);
				}
			}
		}
	}



	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
			   (end.tv_usec - start.tv_usec) / 1e6;
	printf("Interpolation Drawing: %.6f seconds\n", duration);

	printf("Triangles drawn: %zu\n", mesh.num_triangles);
}

void draw_triangle_debug(cairo_t *cr) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for(int i = 0; i < mesh.num_triangles; i++) {
		if(0);
//		else if(is_triangle_edge(*mesh.triangles[i])) cairo_set_source_rgb(cr, 0, 1, 0);
		else if(mesh.triangles[i]->point_flags >> TRI_FLAG_SAVED_BIG & 1) cairo_set_source_rgb(cr, 0, 0.5, 1);
		else if(is_triangle_big(*mesh.triangles[i])) cairo_set_source_rgb(cr, 1, 0, 0);
//		else if(mesh.triangles[i]->point_flags >> TRI_FLAG_IS_NEW & 1) cairo_set_source_rgb(cr, 1, 1, 1);
		else cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);

//		if(!is_triangle_edge(*mesh.triangles[i])) continue;

		cairo_move_to(cr, mesh.triangles[i]->points[0]->data.x, mesh.triangles[i]->points[0]->data.y);
		cairo_line_to(cr, mesh.triangles[i]->points[1]->data.x, mesh.triangles[i]->points[1]->data.y);
		cairo_line_to(cr, mesh.triangles[i]->points[2]->data.x, mesh.triangles[i]->points[2]->data.y);
		cairo_close_path(cr);

		cairo_fill(cr);
	}



	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Interpolation Drawing: %.6f seconds\n", duration);

	printf("Triangles drawn: %zu\n", mesh.num_triangles);
}

void on_draw_mesh_test(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	struct timeval start, end;
	gettimeofday(&start, NULL);
	// reset drawing area
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

	cairo_set_source_surface(cr, image_surface, 0, 0);
	cairo_paint(cr);



//	for(int i = 0; i < mesh.num_triangles; i++) {
//		if(is_triangle_big(*mesh.triangles[i]) && !(mesh.triangles[i]->point_flags >> TRI_FLAG_SAVED_BIG & 1)) {
//			remove_triangle_from_pcmesh(&mesh, i);
//			break;
//		}
//	}
//
//
//	pixel_data = calloc(2000 * 2000, sizeof(uint32_t));
//
//	image_surface = cairo_image_surface_create_for_data(
//			(unsigned char *)pixel_data,
//			CAIRO_FORMAT_RGB24,
//			2000,
//			2000,
//			2000 * 4
//	);
//
//
//	cairo_t *cr1 = cairo_create(image_surface);
//	draw_mesh_interpolated_points(cr1, 2000, 2000);
//	draw_mesh(cr1);
////	draw_triangle_debug(cr1);
//	draw_points(cr1);
//
//
//
//	gettimeofday(&end, NULL);
//	double duration = (end.tv_sec - start.tv_sec) +
//					  (end.tv_usec - start.tv_usec) / 1e6;
//	printf("Drawing: %.6f seconds\n", duration);
}

//struct PcMeshGridGroup {
//	PcMeshGrid grid;
//};

PcMeshPoint * find_prev_in_mesh_grid_group(PcMeshPoint *point) {
	PcMeshPoint *point_prev = NULL;
	double min_dist = 1e9;
	for(int i = 0; i < mesh_grid.num_cols; i++) {
		for(int j = 0; j < mesh_grid.num_col_rows[i]; j++) {
			double ddep_date = mesh_grid.points[i][j]->porkchop_point.dep_date - point->porkchop_point.dep_date;
			double dfb1_dur = (mesh_grid.points[i][j]->porkchop_point.arrival->prev->prev->date - mesh_grid.points[i][j]->porkchop_point.dep_date) - (point->porkchop_point.arrival->prev->prev->date - point->porkchop_point.dep_date);
			double dist = ddep_date*ddep_date + dfb1_dur*dfb1_dur;
			if(dist < 0.1) continue;
			if(dist < min_dist) {
				min_dist = dist;
				point_prev = mesh_grid.points[i][j];
			}
		}
	}

	return point_prev;
}

int i_idx = 0;

void mesh_group_test(cairo_t *cr) {
	cairo_rectangle(cr, 0, 0, 2000, 2000);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

	double *x = malloc(100000*sizeof(double));
	double *y = malloc(100000*sizeof(double));
	double *z = malloc(100000*sizeof(double));
	int num_points = 0;

	for(int i = i_idx; i < mesh_grid.num_cols; i++) {
		for(int j = 0; j < mesh_grid.num_col_rows[i]; j++) {
//			x[num_points] = mesh_grid.points[i][j]->porkchop_point.arrival->prev->prev->date-mesh_grid.points[i][j]->porkchop_point.arrival->prev->prev->prev->date;
//			y[num_points] = mesh_grid.points[i][j]->porkchop_point.arrival->date-mesh_grid.points[i][j]->porkchop_point.arrival->prev->date  -  (mesh_grid.points[i][j-1]->porkchop_point.arrival->date-mesh_grid.points[i][j-1]->porkchop_point.arrival->prev->date);
//			x[num_points] = mesh_grid.points[i][j]->porkchop_point.dep_date;
//			y[num_points] = mesh_grid.points[i][j]->porkchop_point.arrival->prev->prev->date-mesh_grid.points[i][j]->porkchop_point.arrival->prev->prev->prev->date;
////			if(y[num_points] < 175 || y[num_points] > 180) continue;
//			PcMeshPoint *point_prev = find_prev_in_mesh_grid_group(mesh_grid.points[i][j]);
//			double ddep_date = mesh_grid.points[i][j]->porkchop_point.dep_date - point_prev->porkchop_point.dep_date;
//			double dfb1_dur = (mesh_grid.points[i][j]->porkchop_point.arrival->prev->prev->date - mesh_grid.points[i][j]->porkchop_point.dep_date) - (point_prev->porkchop_point.arrival->prev->prev->date - point_prev->porkchop_point.dep_date);
//			double dist = sqrt(ddep_date*ddep_date + dfb1_dur*dfb1_dur);
//
//			x[num_points] = y[num_points];
//			y[num_points] = (mesh_grid.points[i][j]->porkchop_point.arrival->date - mesh_grid.points[i][j]->porkchop_point.dep_date) - (point_prev->porkchop_point.arrival->date - point_prev->porkchop_point.dep_date);
////			if(fabs(y[num_points]) > 20) continue;
//			y[num_points] /= fmin((mesh_grid.points[i][j]->porkchop_point.arrival->prev->date - mesh_grid.points[i][j]->porkchop_point.dep_date), (point_prev->porkchop_point.arrival->prev->date - point_prev->porkchop_point.dep_date));
//			z[num_points] = fmin((mesh_grid.points[i][j]->porkchop_point.arrival->date - mesh_grid.points[i][j]->porkchop_point.dep_date), (point_prev->porkchop_point.arrival->date - point_prev->porkchop_point.dep_date));

			x[num_points] = mesh_grid.points[i][j]->porkchop_point.arrival->prev->prev->date-mesh_grid.points[i][j]->porkchop_point.arrival->prev->prev->prev->date;
			y[num_points] = mesh_grid.points[i][j]->porkchop_point.arrival->date-mesh_grid.points[i][j]->porkchop_point.arrival->prev->date;

			num_points++;
		}
		if(i == i_idx) break;
	}
	i_idx++;

	draw_scatter(cr, 2000, 2000, x, y, z, num_points);

	free(x), free(y), free(z);
}

void on_mesh_drawing_area_pressed() {
	image_surface = cairo_image_surface_create_for_data(
			(unsigned char *)pixel_data,
			CAIRO_FORMAT_RGB24,
			2000,
			2000,
			2000 * 4
	);

	cairo_t *cr = cairo_create(image_surface);

	cairo_rectangle(cr, 0, 0, 2000, 2000);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

//	draw_mesh_interpolated_points(cr, 2000, 2000);
//	draw_triangle_debug(cr);
//	draw_mesh(cr);
	draw_points(cr);

//	mesh_group_test(cr);


	gtk_widget_queue_draw(GTK_WIDGET(mesh_drawing_area));
}


void init_mesh_test() {
	int test_number = 2;

	char filepath[100];
	sprintf(filepath, "../Itineraries/mesh_test%d.itins", test_number);
	int num_arr_per_dep_fb[10];


	for(int i = 0; i < 10; i++) {
		num_arr_per_dep_fb[i] = 0;
	}

	struct ItinsLoadFileResults load_results = load_itineraries_from_bfile(filepath);
	int num_deps = load_results.header.num_deps;
	struct Dv_Filter dv_filter = load_results.header.calc_data.dv_filter;
	struct ItinStep **departures = load_results.departures;

	int num_itins = 0;
	int *num_itins_per_dep = calloc(num_deps, sizeof(int));
	for(int i = 0; i < num_deps; i++) {
		num_itins_per_dep[i] = get_number_of_itineraries(departures[i]);
		num_itins += num_itins_per_dep[i];

		for(int j = 0; j < departures[i]->num_next_nodes; j++) {
			if(get_number_of_itineraries(departures[i]->next[j]) > 1) {
			}
			num_arr_per_dep_fb[get_number_of_itineraries(departures[i]->next[j])]++;
		}
	}

	for(int i = 0; i < 10; i++) {
		printf("% 3d: % 9d\n", i, num_arr_per_dep_fb[i]);
	}

	int index = 0;
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(departures[i], arrivals, &index);
	struct PorkchopPoint *porkchop_points = malloc(num_itins * sizeof(struct PorkchopPoint));
	for(int i = 0; i < num_itins; i++) porkchop_points[i] = create_porkchop_point(arrivals[i], dv_filter.dep_periapsis, dv_filter.arr_periapsis);


	struct timeval start, end;
	gettimeofday(&start, NULL);

	pc_point_groups = create_pcmesh_groups_grom_porkchop(porkchop_points, num_deps, num_itins_per_dep, load_results.header.calc_data);
	mesh_grid = create_pcmesh_grid_from_porkchop(porkchop_points, num_deps, num_itins_per_dep);

	mesh_grid = create_pcmesh_grid_from_pcmesh_group(pc_point_groups.groups[0]);

	mesh = new_mesh();
	for(int i = 0; i < pc_point_groups.num_groups; i++) append_grid_to_pcmesh(&mesh, create_pcmesh_grid_from_pcmesh_group(pc_point_groups.groups[i]), &load_results.header.calc_data);
	printf("num groups: %d\n", pc_point_groups.num_groups);

	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Mesh Creation: %.6f seconds\n", duration);
	gettimeofday(&start, NULL);

	reduce_pcmesh_big_triangles(&mesh, dv_filter);




	gettimeofday(&end, NULL);
	duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Reducing big triangles: %.6f seconds\n", duration);
	gettimeofday(&start, NULL);




//	fine_mesh_around_edge(&mesh, 2, -0.5, dv_filter);

	convert_pcmesh_to_total_dur(mesh);


	gettimeofday(&end, NULL);
	duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Fine Meshing: %.6f seconds\n", duration);
	gettimeofday(&start, NULL);


	resize_pcmesh_to_fit(mesh, 2000, 2000, 1);

	gettimeofday(&end, NULL);
	duration = (end.tv_sec - start.tv_sec) +
					  (end.tv_usec - start.tv_usec) / 1e6;
	printf("Mesh Resizing: %.6f seconds\n", duration);




	pixel_data = calloc(2000 * 2000, sizeof(uint32_t));


//	free(arrivals);
//	free(porkchop_points);

	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect (app, "activate", G_CALLBACK (activate_mesh_test), NULL);

	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
}

void activate_mesh_test(GtkApplication *app, gpointer user_data) {
	/* Construct a GtkBuilder instance and load our UI description */
	setlocale(LC_NUMERIC, "C");	// Glade somehow uses commas instead of points for decimals...
	GtkBuilder *builder = gtk_builder_new ();
	gtk_builder_add_from_file(builder, "../GUI/projection_test.glade", NULL);

	gtk_builder_connect_signals(builder, NULL);

	/* Connect signal handlers to the constructed widgets. */
	GObject *window = gtk_builder_get_object (builder, "window");
	gtk_window_set_application(GTK_WINDOW (window), app);
	gtk_widget_set_visible(GTK_WIDGET (window), TRUE);


	mesh_drawing_area = gtk_builder_get_object(builder, "drawing_area");
//	g_timeout_add(1.0/60*1000.0, on_timeout_mesh_test, mesh_drawing_area);
	gtk_widget_add_events(GTK_WIDGET(mesh_drawing_area),
							GDK_BUTTON_PRESS_MASK |
							GDK_BUTTON_RELEASE_MASK |
							GDK_POINTER_MOTION_MASK |
							GDK_SCROLL_MASK);

	g_signal_connect(mesh_drawing_area, "button-press-event", G_CALLBACK(on_mesh_drawing_area_pressed), NULL);

	on_mesh_drawing_area_pressed();

	/* We do not need the builder anymore */
	g_object_unref(builder);
}
