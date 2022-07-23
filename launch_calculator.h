#ifndef LAUNCH_CALCULATOR
#define LAUNCH_CALCULATOR

// initialize vessel (Thrust: F[kN], initial mass: m0[t], burn rate: br[kg/s])
struct  Vessel init_vessel(double F_vac, double F_sl, double m0, double br);
// initialize flight
struct  Flight init_flight(struct Body *body);
// initialize body
struct  Body init_body();
// Prints parameters specific to the vessel
void    print_vessel_info(struct Vessel *v);
// Prints parameters specific to the flight
void    print_flight_info(struct Flight *f);

// calculate parameters during launch
void    launch_calculator();

// Calculates vessel and flight parameters over time (end is defined by T)
void    calculate_flight(struct Vessel *v, struct Flight *f, double T);
// set starting parameters for flight
void    start_flight(struct Vessel *v, struct Flight *f);
// update parameters of vessel for the point in time t of the flight
void    update_vessel(struct Vessel *v, double t, double p, double h);
// update parameters of the flight for the point in time t of the flight
void    update_flight(struct Vessel *v, struct Vessel *last_v, struct Flight *f, struct Flight *last_f, double t, double step);

// get atmospheric pressure p at height h
double  get_atmo_press(double h);
// calculate acceleration due to aerodynamic drag with given velocity and atmospheric pressure
double  calc_aerodynamic_drag(double p, double v);
// get thrust at current atmosperic pressure
double  get_thrust(struct Vessel *v, double p);
// get vessel's pitch after time t
double  get_pitch(double t);
// get vessel's mass after time t
double  get_ship_mass(struct Vessel *v, double t);
// get vessel's acceleration (Thrust) after time  t
double  get_ship_acceleration(struct Vessel *v, double t);
// get vessel's horizontal acceleration (Thrust) after time t
double  get_ship_hacceleration(struct Vessel *v, double t);
// get vessels's vertical acceleration (Thrust) after time t
double  get_ship_vacceleration(struct Vessel *v, double t);

// calculate centrifugal acceleration due to the vessel's horizontal speed
double  calc_centrifugal_acceleration(struct Flight *f);
// calculate gravitational acceleration at a given distance to the center of the body
double  calc_grav_acceleration(struct Flight *f);
// calculate acceleration towards body without the vessel's thrust (g-ac)
double  calc_balanced_acceleration(struct Flight *f);
// calculate acceleration towards body with vessel's thrust
double  calc_vertical_acceleration(struct Vessel *v, struct Flight *f);
// calculate overall velocity with given vertical and horizontal speed (pythagoras)
double  calc_velocity(double vh, double vv);

// calculate current Apoapsis of flight/orbit
double  calc_Apoapsis(struct Flight *f);

// integration for a given interval (numerical integration, midpoint/rectangle rule)
// I = ( (f(a)+f(b))/2 ) * step
double  integrate(double fa, double fb, double step);
// transforms degrees to radians
double  deg_to_rad(double deg);

// store current flight parameters in addition to the already stored flight parameters
void    store_flight_data(struct Vessel *v, struct Flight *f, double *data);

#endif