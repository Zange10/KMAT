#ifndef CELESTIAL_BODIES
#define CELESTIAL_BODIES

#include "orbit_calculator/orbit.h"

struct Body {
	char name[32];
	double color[3];		// color used for orbit and body visualization
	int id;                 // ID given by JPL's Horizon API
	double mu;              // gravitational parameter of body [m³/s²]
	double radius;          // radius of body [m]
	double rotation_period; // the time period, in which the body rotates around its axis [s]
	double sl_atmo_p;       // atmospheric pressure at sea level [Pa]
	double scale_height;    // the height at which the atmospheric pressure decreases by the factor e [m]
	double atmo_alt;        // highest altitude with atmosphere (ksp-specific) [m]
	int is_homebody;		// home body of system (e.g. Earth or Kerbin)
	struct Vector rot_axis; // Rotation axis pointing to northpole
	struct Vector pm_ut0;	// the prime meridian at ut0
	struct System *system;	// the system the body is the central body of
	struct Orbit orbit;     // orbit of body
	struct Ephem *ephem;	// Ephemerides of body (if available)
};

enum SystemCalcMethod {ORB_ELEMENTS, EPHEMS};

struct System {
	char name[50];
	int num_bodies;
	struct Body *cb;					// central body of system
	struct Body **bodies;				// bodies orbiting the central body
	enum SystemCalcMethod calc_method;	// Propagation using orbital elements or ephemerides
	double ut0;							// time of t0 for system
};

struct Body * new_body();

struct System * new_system();

void parse_and_sort_into_celestial_subsystems(struct System *system);

void init_available_systems(const char *directory);

int get_num_available_systems();

int is_available_system(struct System *system);

int get_number_of_subsystems(struct System *system);

struct System * get_subsystem_from_system_and_id(struct System *system, int id);

struct System * get_top_level_system(struct System *system);

struct System * get_system_by_name(char *name);

struct Body * get_body_by_name(char *name, struct System *system);

void free_all_celestial_systems();

struct System ** get_available_systems();

void free_system(struct System *system);

void print_celestial_system(struct System *system);

/**
 * @brief Initializes all celestial bodies
 */
void init_celestial_bodies();

/**
 * @brief Returns all stored celestial bodies
 */
struct Body ** all_celest_bodies();


/**
 * @brief Returns id of body in system
 */
int get_body_system_id(struct Body *body, struct System *system);


struct Body * SUN();
struct Body * EARTH();

#endif