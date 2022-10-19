#ifndef ORBIT_CALCULATOR
#define ORBIT_CALCULATOR

// construct orbit with the two apsides and the inclination
struct Orbit construct_orbit(double apsis1, double apsis2, double inclination, struct Body body);

// Prints parameters specific to the orbit
void print_orbit_info(struct Orbit o, struct Body body);
// Prints apsides (no line-break): "Apoapsis - Periapsis"
void print_orbit_apsides(double apsis1, double apsis2, struct Body body);

// user chooses, which calculation should be performed
void orbit_calculator();

// user chooses, which calculation should be performed for calculating delta-v requirements of changing orbit
void dv_req_calculator(struct Body parent_body);

// user chooses, which calculation should be performed for in-Orbit parameters
void in_orbit_calculator(struct Body parent_body);

// Choose celestial parent body
struct Body choose_celestial_body(struct Body parent_body);

// calculate and print orbital parameters with given apsides and inclination
void calc_orbital_parameters(struct Body body);

// needed dV to change one apsis of the orbit from a circular orbit
void change_apsis_circ(struct Body body);

// needed dV to change one apsis of the orbit
void change_apsis(struct Body body);

// calculate Delta-V to execute a Hohmann transfer between two circular orbits
void calc_hohmann_transfer(struct Body body);

// calculate needed Delta-V for inclination change
void calc_inclination_change();

// create a plan of one or more maneuvers to reach the planned orbit from the initial orbit
struct ManeuverPlan calc_change_orbit_dV(struct Orbit initial_orbit, struct Orbit planned_orbit, struct Body body);

// calculate the needed Delta-V to execute the maneuver to change orbit
double calc_maneuver_dV(double static_apsis, double initial_apsis, double new_apsis, struct Body body);

// calculate first cosmic speed of parent body
void calc_first_cosmic_speed(struct Body body);

// calculate second cosmic speed of parent body
void calc_second_cosmic_speed(struct Body body);

// calculate speed in circular orbit
void calc_v_at_circ(struct Body body);

// calculate the tangential speed at point in orbit
double calc_orbital_speed(double altitude, double a, struct Body body);

// transforms degrees to radians
double deg2rad(double deg);

#endif
