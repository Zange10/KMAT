#ifndef CELESTIAL_BODIES
#define CELESTIAL_BODIES

#include "orbit.h"


// initialize all celestial bodies
void init_celestial_bodies();

// returns all stored celestial bodies in an array
struct Body ** all_celest_bodies();

struct Body * SUN();
struct Body * VENUS();
struct Body * EARTH();
struct Body * MOON();
struct Body * MARS();
struct Body * JUPITER();
struct Body * SATURN();

struct Body * KERBIN();

#endif