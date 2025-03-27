#ifndef CELESTIAL_BODIES
#define CELESTIAL_BODIES

#include "orbit_calculator/orbit.h"

// struct Body currently declared in orbit.h (needs to be declared after struct Orbit)


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

void init_available_systems(const char *directory);

int get_num_available_systems();

int is_available_system(struct System *system);

void free_all_celestial_systems();

struct System ** get_available_systems();

void free_system(struct System *system);

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