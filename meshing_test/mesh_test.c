#include "mesh_test.h"
#include <stdio.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <math.h>
#include <sys/time.h>
#include "tools/analytic_geometry.h"
#include "gui/drawing.h"
#include "mesh_creator.h"


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
	value /= 1000;
	double r = value;
	double g = 1-value;
	double b = 4*pow(value-0.5,2);
	cairo_set_source_rgb(cr, r,g,b);
}

void get_color_from_value(double value, uint32_t *color) {
	// Normalize to [0, 1]
	value /= 1000.0;
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
	for(int i = 0; i < 3; i++) {
		set_color_from_value(cr, triangle.points[i].z);
		cairo_arc(cr, triangle.points[i].x, triangle.points[i].y, 10, 0, 2*M_PI);
		cairo_fill(cr);
	}
}

void draw_mesh(cairo_t *cr) {
	cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
	for(int i = 0; i < mesh.num_triangles; i++) {
		draw_mesh_triangle(cr, mesh.triangles[i]);
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

//	for(int x = 0; x < 2000; x+=step) {
//		for(int y = 0; y < 2000; y+=step) {
//			struct Vector2D p = vec2D(x, y);
//			MeshTriangle *triangle = find_triangle_for_point(p);
//			if(triangle != NULL) {
//				get_color_from_value(get_triangle_interpolated_value(*triangle, p), &pixel_data[y*width+x]);
//			}
//		}
//	}

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

	draw_mesh_interpolated_points(cr, area_width, area_height);
	draw_mesh(cr);
}


void init_mesh_test() {
	double x_vals[] = {100, 300, 500, 700, 900, 1100, 1250, 1400, 1550, 1700, 1750, 1900};
	int num_cols = sizeof(x_vals) / sizeof(double);

	double *y_vals[] = {
			(double[]){50, 150, 250, 400, 600},                            // 5 points
			(double[]){100, 180, 350, 600, 850, 1200},                     // 6 points
			(double[]){75, 200, 500, 700, 1100},                           // 5 points
			(double[]){100, 300, 550, 900, 1300, 1500},                    // 6 points
			(double[]){200, 400, 750, 1100, 1500, 1950},                   // 6 points
			(double[]){250, 600, 950, 1300, 1650, 1900},                   // 6 points
			(double[]){100, 300, 500, 800, 1150, 1700},                    // 6 points
			(double[]){50, 250, 500, 800, 1100, 1400, 1950},               // 7 points
			(double[]){200, 400, 700, 1050, 1350, 1650, 1950},             // 7 points
			(double[]){300, 600, 900, 1300, 1700},                         // 5 points
			(double[]){150, 400, 750, 1200, 1600, 1680},                   // 6 points
			(double[]){100, 250, 500, 800, 1150, 1500, 1850, 1990}         // 8 points
	};

	int num_points[] = {5, 6, 5, 6, 6, 6, 6, 7, 7, 5, 6, 8};

	double *z_vals[] = {
			(double[]){100, 220, 310, 460, 580},                         // 5
			(double[]){90, 250, 470, 610, 720, 910},                     // 6
			(double[]){80, 190, 520, 700, 990},                          // 5
			(double[]){150, 340, 570, 810, 930, 1000},                   // 6
			(double[]){120, 260, 600, 850, 980, 990},                    // 6
			(double[]){200, 400, 700, 880, 940, 999},                    // 6
			(double[]){170, 300, 500, 790, 910, 980},                    // 6
			(double[]){130, 220, 500, 740, 850, 920, 1000},              // 7
			(double[]){110, 330, 610, 840, 920, 970, 990},               // 7
			(double[]){140, 370, 590, 850, 910},                         // 5
			(double[]){160, 300, 650, 780, 880, 950},                    // 6
			(double[]){190, 310, 540, 770, 890, 930, 970, 999}           // 8
	};


	mesh_grid = create_mesh_grid(x_vals, y_vals, z_vals, num_cols, num_points);

	GtkApplication *app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect (app, "activate", G_CALLBACK (activate_mesh_test), NULL);

	g_application_run (G_APPLICATION (app), 0, NULL);
	g_object_unref (app);
}

static gboolean on_timeout_mesh_test(gpointer data) {
	angle += 0.1;

	for(int i = 0; i < mesh_grid.num_columns; i++) {
		double y_add = sin(angle+i*2)*10;
		for(int j = 0; j < mesh_grid.num_points[i]; j++) {
			y_add += sin(angle+j)*10;
			mesh_grid.points[i][j].y += y_add;
		}
	}

	mesh = create_mesh_from_grid(mesh_grid);
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
