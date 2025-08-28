#ifndef CELESTIAL_BODIES
#define CELESTIAL_BODIES

#include "orbitlib.h"

void init_available_systems(const char *directory);

int get_num_available_systems();

CelestSystem ** get_available_systems();

int is_available_system(CelestSystem *system);

CelestSystem * get_subsystem_from_system_and_id(CelestSystem *system, int id);

CelestSystem * get_system_by_name(char *name);

void free_all_celestial_systems();

/**
 * @brief Initializes all celestial bodies
 */
void init_celestial_bodies();

/**
 * @brief Returns all stored celestial bodies
 */
Body ** all_celest_bodies();


Body * SUN();
Body * EARTH();

#endif