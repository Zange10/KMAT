#include "analytic_geometry.h"
#include <math.h>
#include <stdio.h>

double vector_mag(struct Vector v) {
    return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

double vector2d_mag(struct Vector2D v) {
    return sqrt(v.x*v.x + v.y*v.y);
}

struct Vector norm_vector(struct Vector v) {
    double mag = vector_mag(v);
    return scalar_multiply(v, 1 / mag);
}

struct Vector2D norm_vector2d(struct Vector2D v) {
    double mag = vector2d_mag(v);
    return scalar_multipl2d(v, 1 / mag);
}

struct Vector2D rotate_vector2d(struct Vector2D n, double gamma) {
    struct Vector2D v_rot;
    gamma *= -1; // clockwise rotation
    v_rot.x = cos(gamma) * n.x - sin(gamma) * n.y;
    v_rot.y = sin(gamma) * n.x + cos(gamma) * n.y;
    return v_rot;
}

struct Vector scalar_multiply(struct Vector v, double scalar) {
    v.x *= scalar;
    v.y *= scalar;
    v.z *= scalar;
    return v;
}

struct Vector2D scalar_multipl2d(struct Vector2D v, double scalar) {
    v.x *= scalar;
    v.y *= scalar;
    return v;
}

double dot_product(struct Vector v1, struct Vector v2) {
    return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

double dot_product2d(struct Vector2D v1, struct Vector2D v2) {
    return v1.x*v2.x + v1.y*v2.y;
}

struct Vector cross_product(struct Vector v1, struct Vector v2) {
    struct Vector v;
    v.x = v1.y*v2.z - v1.z*v2.y;
    v.y = v1.z*v2.x - v1.x*v2.z;
    v.z = v1.x*v2.y - v1.y*v2.x;
    return v;
}

struct Plane constr_plane(struct Vector loc, struct Vector u, struct Vector v) {
    struct Plane p;
    p.loc = loc;
    p.u = u;
    p.v = v;
    return p;
}

struct Vector2D inverse_vector2d(struct Vector2D v) {
    v.x = 1/v.x;
    v.y = 1/v.y;
    return v;
}

double angle_vec_vec(struct Vector v1, struct Vector v2) {
    return fabs(acos(dot_product(v1,v2) / (vector_mag(v1)* vector_mag(v2))));
}

double angle_plane_vec(struct Plane p, struct Vector v) {
    struct Vector n = cross_product(p.u, p.v);
    return M_PI - angle_vec_vec(n, v);
}

double angle_plane_plane(struct Plane p1, struct Plane p2) {
    struct Vector n1 = cross_product(p1.u, p1.v);
    struct Vector n2 = cross_product(p2.u, p2.v);
    return angle_vec_vec(n1, n2);
}

double angle_vec_vec_2d(struct Vector2D v1, struct Vector2D v2) {
    return fabs(acos(dot_product2d(v1,v2) / (vector2d_mag(v1)* vector2d_mag(v2))));
}

void print_vector(struct Vector v) {
    printf("\n%f, %f, %f\n", v.x, v.y, v.z);
    printf("\nx: %f\ny: %f\nz: %f\n", v.x, v.y, v.z);
}

void print_vector2d(struct Vector2D v) {
    printf("\nx: %f\ny: %f\n", v.x, v.y);
}

double deg2rad(double deg) {
    return deg/180*M_PI;
}

double rad2deg(double rad) {
    return rad/M_PI*180;
}
