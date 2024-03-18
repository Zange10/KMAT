//
// Created by niklas on 18.03.24.
//

#ifndef KSP_ITIN_TOOL_H
#define KSP_ITIN_TOOL_H

#include "tools/analytic_geometry.h"
#include "ephem.h"
#include <stdio.h>

struct ItinStep {
	struct Body *body;
	struct Vector r;
	struct Vector v_arr, v_body, v_dep;
	double date;
	int num_next_nodes;
	struct ItinStep *prev;
	struct ItinStep **next;
};



// find viable flybys to next body with a given arrival trajectory
void find_viable_flybys(struct ItinStep *tf, struct Ephem **ephems, struct Body *next_body, double min_dt, double max_dt);

// find viable flybys to next body with a given arrival trajectory
void find_viable_dsb_flybys(struct ItinStep *tf, struct Ephem **ephems, struct Body *next_body, double min_dt0, double max_dt0, double min_dt1, double max_dt1);

// print itinerary dates (arrival first)
void print_itinerary(struct ItinStep *itin);

// returns the number of itineraries (departure first)
int get_number_of_itineraries(struct ItinStep *itin);

// returns number of total amount of stored ininerary steps (departure first)
int get_total_number_of_stored_steps(struct ItinStep *itin);

// stores all arrival nodes in array from itin (departure first)
void store_itineraries_in_array(struct ItinStep *itin, struct ItinStep **array, int *index);

// returns the itinerary duration in days (arrival first)
double get_itinerary_duration(struct ItinStep *itin);

// add itinerary departure date, duration, departure dv, deep-space maneuvre dv and arrival dv in porkchop array
void create_porkchop_point(struct ItinStep *itin, double* porkchop, int circ_cap_fb);

// from current step and given information, initiate calculation of next steps
int calc_next_step(struct ItinStep *curr_step, struct Ephem **ephems, struct Body **bodies, const int *min_duration, const int *max_duration, int num_steps, int step);

// get the number of layers/steps in the given itinerary (departure first)
int get_num_of_itin_layers(struct ItinStep *step);

// store itineraries in text file from multiple departures (pre-order storing)
void store_itineraries_in_file_init(struct ItinStep **departures, int num_nodes, int num_deps);

// store itineraries in binary file from multiple departures (pre-order storing)
void store_itineraries_in_bfile_init(struct ItinStep **departures, int num_nodes, int num_deps);

// load itineraries from binary file for multiple departures (from pre-order storing)
struct ItinStep ** load_itineraries_from_bfile_init();

// removes this and all now unneeded steps from itineraries (no next node before arrival)
void remove_step_from_itinerary(struct ItinStep *step);

// free to itinerary allocated memory
void free_itinerary(struct ItinStep *step);



#endif //KSP_ITIN_TOOL_H
