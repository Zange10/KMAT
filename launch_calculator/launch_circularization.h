#ifndef KSP_LAUNCH_CIRCULARIZATION_H
#define KSP_LAUNCH_CIRCULARIZATION_H

// returns the pitch for the circularization stage to get vertical speed to 0 at circular orbit
double circularization_pitch(double F, double m0, double br, double vh_0, double vv_0, double r, double mu);

#endif //KSP_LAUNCH_CIRCULARIZATION_H