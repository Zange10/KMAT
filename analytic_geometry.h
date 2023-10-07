#ifndef KSP_ANALYTIC_GEOMETRY_H
#define KSP_ANALYTIC_GEOMETRY_H

struct Vector {
    double x;
    double y;
    double z;
};

struct Vector2D {
    double x;
    double y;
};

struct Plane {
    struct Vector loc;
    struct Vector u;
    struct Vector v;
};

struct Vector add_vectors(struct Vector v1, struct Vector v2);

double vector_mag(struct Vector v);

double vector2d_mag(struct Vector2D v);

struct Vector norm_vector(struct Vector v);

struct Vector2D norm_vector2d(struct Vector2D v);

struct Vector2D rotate_vector2d(struct Vector2D n, double gamma);

struct Vector scalar_multiply(struct Vector v, double scalar);

struct Vector2D scalar_multipl2d(struct Vector2D v, double scalar);

double dot_product(struct Vector v1, struct Vector v2);

double dot_product2d(struct Vector2D v1, struct Vector2D v2);

struct Vector cross_product(struct Vector v1, struct Vector v2);

struct Plane constr_plane(struct Vector loc, struct Vector u, struct Vector v);

struct Vector2D inverse_vector2d(struct Vector2D v);

double angle_vec_vec(struct Vector v1, struct Vector v2);

double angle_plane_vec(struct Plane p, struct Vector v);

double angle_plane_plane(struct Plane p1, struct Plane p2);

double angle_vec_vec_2d(struct Vector2D v1, struct Vector2D v2);

void print_vector(struct Vector v);

void print_vector2d(struct Vector2D v);

double deg2rad(double deg);

double rad2deg(double rad);

#endif //KSP_ANALYTIC_GEOMETRY_H
