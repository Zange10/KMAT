#ifndef CELESTIAL_BODIES
#define CELESTIAL_BODIES

#include "orbit_calculator/orbit.h"


// struct Body currently declared in orbit.h (needs to be declared after struct Orbit)

// initialize all celestial bodies
void init_celestial_bodies();

// returns all stored celestial bodies in an array
struct Body ** all_celest_bodies();

struct Body * get_body_from_id(int id);

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

struct Body * KERBOL();
struct Body * KERBIN();

#endif