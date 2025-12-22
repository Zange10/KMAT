#ifndef KSP_LAUNCH_STATE_H
#define KSP_LAUNCH_STATE_H

#include "geometrylib.h"

struct LaunchState {
	double t;			// time since launch [s]
	Vector3 r;	// position [m]
	Vector3 v;	// velocity [m/s]
	double m;			// vessel mass [kg]
	double pitch;
	int stage_id;
	struct LaunchState *prev;
	struct LaunchState *next;
};

struct LaunchState * new_launch_state();

struct LaunchState * append_launch_state(struct LaunchState *curr_state);

struct LaunchState * get_first_state(struct LaunchState *state);

struct LaunchState * get_last_state(struct LaunchState *state);

void free_launch_states(struct LaunchState *state);

#endif //KSP_LAUNCH_STATE_H
