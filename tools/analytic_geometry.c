#include "analytic_geometry.h"
#include <math.h>
#include <stdio.h>


struct Vector vec(double x, double y, double z) {
    struct Vector v = {x, y, z};
    return v;
}

struct Vector add_vectors(struct Vector v1, struct Vector v2) {
    struct Vector v;
    v.x = v1.x+v2.x;
    v.y = v1.y+v2.y;
    v.z = v1.z+v2.z;
    return v;
}

struct Vector subtract_vectors(struct Vector v1, struct Vector v2) {
	struct Vector v;
	v.x = v1.x-v2.x;
	v.y = v1.y-v2.y;
	v.z = v1.z-v2.z;
	return v;
}

struct Vector2D add_vectors2d(struct Vector2D v1, struct Vector2D v2) {
	struct Vector2D v;
	v.x = v1.x+v2.x;
	v.y = v1.y+v2.y;
	return v;
}

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

struct Vector2D rotate_vector2d(struct Vector2D v, double gamma) {
    struct Vector2D v_rot;
    gamma *= -1; // clockwise rotation
    v_rot.x = cos(gamma)*v.x - sin(gamma)*v.y;
    v_rot.y = sin(gamma)*v.x + cos(gamma)*v.y;
    return v_rot;
}

struct Vector scalar_multiply(struct Vector v, double scalar) {
    v.x *= scalar;
    v.y *= scalar;
    v.z *= scalar;
    return v;
}

struct Vector proj_vec_plane(struct Vector v, struct Plane p) {
	struct Vector n = norm_vector(cross_product(p.u, p.v));
	struct Vector proj_n = scalar_multiply(n, dot_product(v,n));
	return subtract_vectors(v, proj_n);
}

struct Vector2D scalar_multipl2d(struct Vector2D v, double scalar) {
    v.x *= scalar;
    v.y *= scalar;
    return v;
}

double determinant2d(struct Vector2D v1, struct Vector2D v2) {
	return v1.x*v2.y - v1.y*v2.x;
}

struct Vector2D vec2d_proj(struct Vector2D v1, struct Vector2D v2) {
	v2 = norm_vector2d(v2);
	return scalar_multipl2d(v2, dot_product2d(v1,v2));
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

struct Vector calc_plane_norm_vector(struct Plane p) {
    return cross_product(p.v, p.u);
}

double angle_vec_vec(struct Vector v1, struct Vector v2) {
	double acos_part = dot_product(v1,v2) / (vector_mag(v1)* vector_mag(v2));
	// some small imprecisions can lead to acos(1.0000....01) (nan) -> rounding to 1/-1
	if(acos_part >  1) acos_part = 1;
	if(acos_part < -1) acos_part = -1;
	double angle = fabs(acos(acos_part));
	return angle;
}

double angle_plane_vec(struct Plane p, struct Vector v) {
    struct Vector n = cross_product(p.u, p.v);
    return M_PI/2 - angle_vec_vec(n, v);
}

double angle_plane_plane(struct Plane p1, struct Plane p2) {
    struct Vector n1 = cross_product(p1.u, p1.v);
    struct Vector n2 = cross_product(p2.u, p2.v);
    return angle_vec_vec(n1, n2);
}

double angle_vec_vec_2d(struct Vector2D v1, struct Vector2D v2) {
    return fabs(acos(dot_product2d(v1,v2) / (vector2d_mag(v1)* vector2d_mag(v2))));
}

double ccw_angle_vec_vec_2d(struct Vector2D v1, struct Vector2D v2) {
	double angle = fabs(acos(dot_product2d(v1,v2) / (vector2d_mag(v1)* vector2d_mag(v2))));
	if(determinant2d(v1, v2) > 0) angle *= -1;
	return angle;
}

struct Vector calc_intersecting_line_dir(struct Plane p1, struct Plane p2) {
    double M[3][5] = {
            {p1.u.x, p1.v.x, -p2.u.x, -p2.v.x, p2.loc.x-p1.loc.x},
            {p1.u.y, p1.v.y, -p2.u.y, -p2.v.y, p2.loc.y-p1.loc.y},
            {p1.u.z, p1.v.z, -p2.u.z, -p2.v.z, p2.loc.z-p1.loc.z}
    };

    // make 0-triangle
    for(int i = 4; i >= 0; i--) M[1][i] = M[0][0]*M[1][i] - M[1][0] * M[0][i];  // I  -> II
    for(int i = 4; i >= 0; i--) M[2][i] = M[0][0]*M[2][i] - M[2][0] * M[0][i];  // I  -> III
    for(int i = 4; i >= 0; i--) M[2][i] = M[1][1]*M[2][i] - M[2][1] * M[1][i];  // II -> III

    double a = M[2][2];
    double b = M[2][3];
    double c = M[2][4];

    // if planes are equal (inside ecliptic for p_0)
    // return p2.u because for earth ecliptic cases draw line from Sun to Earth
    if(a == 0) return scalar_multiply(p2.u, 1);

    /*
    struct Vector loc = {
            p2.loc.x + c/a*p2.u.x,
            p2.loc.y + c/a*p2.u.y,
            p2.loc.z + c/a*p2.u.z
    }; */

    struct Vector dir = {
            -b/a*p2.u.x + p2.v.x,
            -b/a*p2.u.y + p2.v.y,
            -b/a*p2.u.z + p2.v.z,
    };

    return dir;
}

struct Vector rotate_vector_around_axis(struct Vector v, struct Vector axis, double angle) {
    struct Vector u = norm_vector(axis);
    double ca = cos(angle);
    double mca = 1-cos(angle);
    double sa = sin(angle);

    double R[3][3] = {
            {ca+u.x*u.x*mca,        u.x*u.y*mca-u.z*sa,         u.x*u.z*mca+u.y*sa},
            {u.y*u.x*mca+u.z*sa,    ca+u.y*u.y*mca,             u.y*u.z*mca-u.x*sa},
            {u.z*u.x*mca-u.y*sa,    u.z*u.y*mca+u.x*sa,         ca+u.z*u.z*mca}
    };

    double v_vec[3] = {v.x, v.y, v.z};
    double rot_v[3] = {0,0,0};

    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            rot_v[i] += R[i][j]*v_vec[j];
        }
    }
    struct Vector rotated_vector = {rot_v[0], rot_v[1], rot_v[2]};
    return rotated_vector;
}


void print_vector(struct Vector v) {
    printf("\n%f, %f, %f (%f)\n", v.x, v.y, v.z, vector_mag(v));
    printf("x: %f\ny: %f\nz: %f\n", v.x, v.y, v.z);
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

double pi_norm(double rad) {
    while(rad >= 2*M_PI) rad -= 2*M_PI;
    while(rad < 0) rad += 2*M_PI;
    return rad;
}
