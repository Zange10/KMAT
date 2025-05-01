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
Mesh mesh;
MeshGrid mesh_grid;

double angle = 0;


typedef struct {
	struct Vector2D points[3];
} MeshTriangle2D;

MeshTriangle2D convert_mesh_triangle_3d_to_2d(MeshTriangle tri3d) {
	MeshTriangle2D tri2d;
	for(int i = 0; i < 3; i++) tri2d.points[i] = vec2D(tri3d.points[i].x, tri3d.points[i].y);
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

MeshTriangle * find_triangle_for_point(struct Vector2D p) {
	for(int i = 0; i < mesh.num_triangles; i++) {
		if(is_inside_triangle(convert_mesh_triangle_3d_to_2d(mesh.triangles[i]), p)) {
			return &mesh.triangles[i];
		}
	}
	return NULL;
}

double get_triangle_interpolated_value(MeshTriangle triangle, struct Vector2D p) {
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

	return u*triangle.points[0].z + v*triangle.points[1].z + w*triangle.points[2].z;
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

void draw_mesh_triangle(cairo_t *cr, MeshTriangle triangle) {
	cairo_set_source_rgb(cr, 1,1,1);
	for(int i = 0; i < 3; i++) {
		draw_stroke(cr, vec2D(triangle.points[i].x, triangle.points[i].y), vec2D(triangle.points[(i+1)%3].x, triangle.points[(i+1)%3].y));
	}
}

void draw_mesh(cairo_t *cr) {
	cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
	for(int i = 0; i < mesh.num_triangles; i++) {
		draw_mesh_triangle(cr, mesh.triangles[i]);
	}
	printf("Triangles drawn: %zu\n", mesh.num_triangles);
}

void draw_points(cairo_t *cr) {
	for(int c = 0; c < mesh.num_triangles; c++) {
		MeshTriangle triangle = mesh.triangles[c];
		for(int i = 0; i < 3; i++) {
			set_color_from_value(cr, triangle.points[i].z);
			cairo_arc(cr, triangle.points[i].x, triangle.points[i].y, 2, 0, 2*M_PI);
			cairo_fill(cr);
		}
	}
}

void draw_mesh_interpolated_points(cairo_t *cr, int width, int height) {
	struct timeval start, end;
	gettimeofday(&start, NULL);

	uint32_t *pixel_data = calloc(width * height, sizeof(uint32_t));
	int step = 1;

	for(int i = 0; i < mesh.num_triangles; i++) {
		double min_x, max_x, min_y, max_y;
		MeshTriangle2D tri2d = convert_mesh_triangle_3d_to_2d(mesh.triangles[i]);
		find_2dtriangle_minmax(tri2d, &min_x, &max_x, &min_y, &max_y);
		if(max_x < 0 || min_x > width || max_y < 0 || min_y > height) continue;
		for(int x = (int)min_x+1; x <= max_x; x+=step) {
			for(int y = (int)min_y+1; y <= max_y; y+=step) {
				struct Vector2D p = vec2D(x, y);
				if(x >= 0 && x < width && y >= 0 && y < height && is_inside_triangle(tri2d, p)) {
					get_color_from_value(get_triangle_interpolated_value(mesh.triangles[i], p), &pixel_data[y*width+x]);
				}
			}
		}
	}

	cairo_surface_t *image_surface = cairo_image_surface_create_for_data(
			(unsigned char *)pixel_data,
			CAIRO_FORMAT_RGB24,
			width,
			height,
			width * 4
	);
	cairo_set_source_surface(cr, image_surface, 0, 0);
	cairo_paint(cr);


	gettimeofday(&end, NULL);
	double duration = (end.tv_sec - start.tv_sec) +
			   (end.tv_usec - start.tv_usec) / 1e6;
	printf("Execution time2: %.6f seconds\n", duration);
	gettimeofday(&start, NULL);

	printf("Triangles drawn: %zu\n", mesh.num_triangles);
	free(pixel_data);
}

void on_draw_mesh_test(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	// reset drawing area
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	int area_width = allocation.width;
	int area_height = allocation.height;

	struct Vector2D center = {(double)area_width/2, (double)area_height/2};

	cairo_rectangle(cr, 0, 0, area_width, area_height);
	cairo_set_source_rgb(cr, 0,0,0);
	cairo_fill(cr);

//	draw_mesh_interpolated_points(cr, area_width, area_height);
	draw_mesh(cr);
//	draw_points(cr);
}


void init_mesh_test() {
	char filepath[] = "../Itineraries/mesh_test1.itins";

	struct ItinsLoadFileResults load_results = load_itineraries_from_bfile(filepath);
	int num_deps = load_results.num_deps;
	struct ItinStep **departures = load_results.departures;

	int num_itins = 0;
	int *num_itins_per_dep = calloc(num_deps, sizeof(int));
	for(int i = 0; i < num_deps; i++) {
		num_itins_per_dep[i] = get_number_of_itineraries(departures[i]);
		num_itins += num_itins_per_dep[i];
	}

	int index = 0;
	struct ItinStep **arrivals = (struct ItinStep**) malloc(num_itins * sizeof(struct ItinStep*));
	for(int i = 0; i < num_deps; i++) store_itineraries_in_array(departures[i], arrivals, &index);
	struct PorkchopPoint *porkchop_points = malloc(num_itins * sizeof(struct PorkchopPoint));
	for(int i = 0; i < num_itins; i++) porkchop_points[i] = create_porkchop_point(arrivals[i]);

//	mesh_grid = grid_from_porkchop(porkchop_points, num_itins, num_deps, num_itins_per_dep);

	mesh = mesh_from_porkchop(porkchop_points, num_itins, num_deps, num_itins_per_dep);

	free(arrivals);
	free(porkchop_points);

	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect (app, "activate", G_CALLBACK (activate_mesh_test), NULL);

	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
}

static gboolean on_timeout_mesh_test(gpointer data) {
	gtk_widget_queue_draw(GTK_WIDGET(mesh_drawing_area));
	return G_SOURCE_CONTINUE;
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

	g_timeout_add(1.0/60*1000.0, on_timeout_mesh_test, mesh_drawing_area);

	/* We do not need the builder anymore */
	g_object_unref(builder);
}
