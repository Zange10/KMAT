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

void store_system_in_config_file(struct System *system);

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
 * @brief Finds the celestial body with the given id (if not found, retuns NULL-pointer)
 */
struct Body * get_body_from_id(int id);


/**
 * @brief Returns id of body in system
 */
int get_body_system_id(struct Body *body, struct System *system);


/**
 * @brief Returns the current system with central and orbiting bodies
 */
struct System * get_current_system();

struct Body * SUN();
struct Body * MERCURY();
struct Body * VENUS();
struct Body * EARTH();
struct Body * MOON();
struct Body * MARS();
struct Body * JUPITER();
struct Body * SATURN();
struct Body * URANUS();
struct Body * NEPTUNE();
struct Body * PLUTO();
struct System * SOLAR_SYSTEM_EPHEM();

struct Body * KERBOL();
struct Body * KERBIN();

#endif