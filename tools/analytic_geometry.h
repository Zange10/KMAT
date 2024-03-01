#ifndef KSP_ANALYTIC_GEOMETRY_H
#define KSP_ANALYTIC_GEOMETRY_H

/**
 * @brief Represents a 3D vector with x, y, and z components
 */
struct Vector {
	double x; /**< X component of the vector */
	double y; /**< Y component of the vector */
	double z; /**< Z component of the vector */
};

/**
 * @brief Represents a 2D vector with x and y components
 */
struct Vector2D {
	double x; /**< X component of the 2D vector */
	double y; /**< Y component of the 2D vector */
};

/**
 * @brief Represents a plane in 3D space defined by a location vector and two directional vectors
 */
struct Plane {
	struct Vector loc; /**< Location vector of the plane */
	struct Vector u;   /**< Directional vector 'u' */
	struct Vector v;   /**< Directional vector 'v' */
};


/**
 * @brief Creates a vector with given x, y, and z components
 *
 * @param x component of the vector
 * @param y component of the vector
 * @param z component of the vector
 * @return A struct Vector initialized with the given components
 */
struct Vector vec(double x, double y, double z);


/**
 * @brief Adds two vectors and returns the result
 *
 * @param v1 The first vector
 * @param v2 The second vector
 * @return A struct Vector where each component of the vectors was added together
 */
struct Vector add_vectors(struct Vector v1, struct Vector v2);


/**
 * @brief Adds two 2D vectors and returns the result
 *
 * @param v1 The first vector
 * @param v2 The second vector
 * @return A 2D Vector where each component of the vectors was added together
 */
struct Vector2D add_vectors2d(struct Vector2D v1, struct Vector2D v2);

/**
 * @brief Returns the magnitude of a given vector
 *
 * @param v The vector with respective magnitude
 * @return The magnitude of the given vector
 */
double vector_mag(struct Vector v);


/**
 * @brief Returns the magnitude of a given 2D-vector
 *
 * @param v The 2D-vector with respective magnitude
 * @return The magnitude of the 2D-given vector
 */
double vector2d_mag(struct Vector2D v);


/**
 * @brief Normalizes a given vector
 *
 * @param v The vector that is to be normalized
 * @return The normalized form of the given vector
 */
struct Vector norm_vector(struct Vector v);


/**
 * @brief Normalizes a given 2D-vector
 *
 * @param v The 2D-vector that is to be normalized
 * @return The normalized form of the given 2D-vector
 */
struct Vector2D norm_vector2d(struct Vector2D v);


/**
 * @brief Rotates a vector around an axis
 *
 * @param v The vector that is to be rotated
 * @param axis The axis-vector around which the vector is to be rotated
 * @param angle The angle by which the vector is to be rotated
 * @return The rotated vector
 */
struct Vector rotate_vector_around_axis(struct Vector v, struct Vector axis, double angle);


/**
 * @brief Rotates 2D-vector in clock-wise direction
 *
 * @param v The 2D-vector that is to be rotated
 * @param gamma The angle by which the vector is to be rotated
 * @return The rotated 2D-vector
 */
struct Vector2D rotate_vector2d(struct Vector2D v, double gamma);


/**
 * @brief Multiplies a vector by a scalar
 *
 * @param v The vector that is to be multiplied
 * @param scalar The amount by which the vector is to be multiplied by
 * @return The scaled vector
 */
struct Vector scalar_multiply(struct Vector v, double scalar);


/**
 * @brief Multiplies a 2D-vector by a scalar
 *
 * @param v The 2D-vector that is to be multiplied
 * @param scalar The amount by which the vector is to be multiplied by
 * @return The scaled 2D-vector
 */
struct Vector2D scalar_multipl2d(struct Vector2D v, double scalar);


/**
 * @brief Calculates the determinant of two 2D-vectors
 *
 * @param v1 vector 1
 * @param v2 vector 2
 * @return The determinant of v1 and v2
 */
double determinant2d(struct Vector2D v1, struct Vector2D v2);


/**
 * @brief Calculates the projection of v1 onto v2
 *
 * @param v1 vector 1
 * @param v2 vector 2
 * @return The projection of v1 onto v2
 */
struct Vector2D vec2d_proj(struct Vector2D v1, struct Vector2D v2);


/**
 * @brief Calculates the dot product of two vectors
 *
 * @param v1 Vector 1
 * @param v2 Vector 2
 * @return The resulting dot product v1 ⋅ v2
 */
double dot_product(struct Vector v1, struct Vector v2);


/**
 * @brief Calculates the dot product of two 2D-vectors
 *
 * @param v1 Vector 1 (2D-vector)
 * @param v2 Vector 2 (2D-vector)
 * @return The resulting dot product (v1 ⋅ v2)
 */
double dot_product2d(struct Vector2D v1, struct Vector2D v2);


/**
 * @brief Calculates the cross product of two vectors
 *
 * @param v1 Vector 1
 * @param v2 Vector 2
 * @return The resulting cross product (v1 x v2)
 */
struct Vector cross_product(struct Vector v1, struct Vector v2);


/**
 * @brief Constructs a plane with a given location and two given direction vectors
 *
 * @param loc Location vector
 * @param u direction vector
 * @param v direction vector
 * @return The constructed plane
 */
struct Plane constr_plane(struct Vector loc, struct Vector u, struct Vector v);


/**
 * @brief Calculates the normal vector of a plane
 *
 * @param p The given plane
 * @return The normal vector of the given plane
 */
struct Vector calc_plane_norm_vector(struct Plane p);


/**
 * @brief Calculates the angle between two vectors
 *
 * @param v1 Vector 1
 * @param v2 Vector 2
 * @return The angle between the two vectors
 */
double angle_vec_vec(struct Vector v1, struct Vector v2);


/**
 * @brief Calculates the angle between a plane and a vector
 *
 * @param p The plane
 * @param v The vector
 * @return The angle between the plane and the vector
 */
double angle_plane_vec(struct Plane p, struct Vector v);


/**
 * @brief Calculates the angle between two plane
 *
 * @param p1 Plane 1
 * @param p2 Plane 2
 * @return The angle between the two planes
 */
double angle_plane_plane(struct Plane p1, struct Plane p2);


/**
 * @brief Calculates the angle between two 2D-vectors in counterclock-wise direction (-pi < angle < pi)
 *
 * @param v1 Vector 1 (2D-vector)
 * @param v2 Vector 2 (2D-vector)
 * @return The angle between the two 2D-vectors (signed)
 */
double ccw_angle_vec_vec_2d(struct Vector2D v1, struct Vector2D v2);


/**
 * @brief Calculates the angle between two 2D-vectors
 *
 * @param v1 Vector 1 (2D-vector)
 * @param v2 Vector 2 (2D-vector)
 * @return The angle between the two 2D-vectors (unsigned)
 */
double angle_vec_vec_2d(struct Vector2D v1, struct Vector2D v2);


/**
 * @brief Calculates the angle between two 2D-vectors
 *
 * @param v1 Vector 1 (2D-vector)
 * @param v2 Vector 2 (2D-vector)
 * @return The angle between the two 2D-vectors
 */
struct Vector calc_intersecting_line_dir(struct Plane p1, struct Plane p2);


/**
 * @brief Prints vector components and magnitude
 *
 * @param v The vector to be printed
 */
void print_vector(struct Vector v);


/**
 * @brief Prints 2D-vector components and magnitude
 *
 * @param v The vector to be printed
 */
void print_vector2d(struct Vector2D v);


/**
 * @brief Converts degrees to radians
 *
 * @param deg Angle in degrees
 * @return Angle in radians
 */
double deg2rad(double deg);


/**
 * @brief Converts degrees to radians
 *
 * @param rad Angle in radians
 * @return Angle in degrees
 */
double rad2deg(double rad);


/**
 * @brief Normalizes angle (in radians) to values between 0 and 2π
 *
 * @param rad Value to be normalized
 * @return Normalized value (0 ≤ value ⋖ 2π)
 */
double pi_norm(double rad);

#endif //KSP_ANALYTIC_GEOMETRY_H
