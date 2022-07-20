#ifndef ORBIT_CALCULATOR
#define ORBIT_CALCULATOR

// Prints parameters specific to the orbit
void print_orbit_info(struct Orbit o);

// user chooses, which calculation should be performed
void choose_calculation();

// needed dV to change one apsis of the orbit from a circular orbit
double change_apsis_circ();

// needed dV to change one apsis of the orbit
double change_apsis();

// calculate semi-major axis of given orbit
double calc_semimajor_axis(struct Orbit o);

// calculate the tangential speed at point in orbit
double calc_orbital_speed(double altitude, double a);

#endif
