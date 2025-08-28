#ifndef KSP_ITIN_TOOL_H
#define KSP_ITIN_TOOL_H

#include "tools/celestial_systems.h"
#include <stdio.h>

struct ItinStep {
	Body *body;
	Vector3 r;
	Vector3 v_arr, v_body, v_dep;
	double date;
	int num_next_nodes;
	struct ItinStep *prev;
	struct ItinStep **next;
};

enum ItinSequenceInfoType {ITIN_SEQ_INFO_TO_TARGET, ITIN_SEQ_INFO_SPEC_SEQ};

typedef union ItinSequenceInfo {
	struct ItinSequenceInfoToTarget {
		enum ItinSequenceInfoType type;
		CelestSystem *system;
		Body *dep_body, *arr_body, **flyby_bodies;
		int num_flyby_bodies;
	} to_target;

	struct ItinSequenceInfoSpecItin {
		enum ItinSequenceInfoType type;
		CelestSystem *system;
		struct Body **bodies;
		int num_steps;
	} spec_seq;
} ItinSequenceInfo;

struct Dv_Filter {
	double max_totdv, max_depdv, max_satdv;
	double dep_periapsis, arr_periapsis;
	int last_transfer_type;
};

struct PorkchopPoint {
		struct ItinStep *arrival;
		double dep_date, dur;
		double dv_dep, dv_dsm, dv_arr_cap, dv_arr_circ;
};


enum LastTransferType {TF_FLYBY, TF_CAPTURE, TF_CIRC};

// find viable flybys to next body with a given arrival trajectory
void find_viable_flybys(struct ItinStep *tf, CelestSystem *system, Body *next_body, double min_dt, double max_dt);

// find viable flybys to next body with a given arrival trajectory
void find_viable_dsb_flybys(struct ItinStep *tf, Ephem **ephems, Body *next_body, double min_dt0, double max_dt0, double min_dt1, double max_dt1);

// return departure step
struct ItinStep * get_first(struct ItinStep *tf);

// return last step from itinerary (first branch)
struct ItinStep * get_last(struct ItinStep *tf);

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

// returns an array of porkchop points analyzed from the given departures (allocates porkchop array memory --> needs to be freed)
struct PorkchopPoint *create_porkchop_array_from_departures(struct ItinStep **departures, int num_deps, double dep_periapsis, double arr_periapsis);

// add itinerary departure date, duration, departure dv, deep-space maneuvre dv and arrival dv in porkchop array
struct PorkchopPoint create_porkchop_point(struct ItinStep *itin, double dep_periapsis, double arr_periapsis);

// from current step and given information, initiate calculation of next steps
int calc_next_spec_itin_step(struct ItinStep *curr_step, CelestSystem *system, Body **bodies, double jd_max_arr, struct Dv_Filter *dv_filter, int num_steps, int step);

// from current step and given information, initiate calculation of next steps
int calc_next_itin_to_target_step(struct ItinStep *curr_step, struct ItinSequenceInfoToTarget *seq_info, double jd_max_arr, double max_total_duration, struct Dv_Filter *dv_filter);

// initiate calc of next itinerary steps and return 0 if no steps are remaining in this itinerary (1 otherwise)
int continue_to_next_steps_and_check_for_valid_itins(struct ItinStep *curr_step, int num_of_end_nodes, struct ItinSequenceInfoToTarget *seq_info, double jd_max_arr, double max_total_duration, struct Dv_Filter *dv_filter);

// find end nodes (next step at arrival body) and copy to the end of the next steps array of curr_step
int find_copy_and_store_end_nodes(struct ItinStep *curr_step, Body *arr_body);

// removes end nodes from initerary that do not satisfy dv requirements (returns new number of end nodes)
int remove_end_nodes_that_do_not_satisfy_dv_requirements(struct ItinStep *curr_step, int num_of_end_nodes, struct Dv_Filter *dv_filter);

// get the number of layers/steps in the given itinerary (departure first)
int get_num_of_itin_layers(struct ItinStep *step);

// update r and v_body vectors of itinerary steps (departure first)
void update_itin_body_osvs(struct ItinStep *step, CelestSystem *system);

// calculate from velocity vectors for itinerary steps from date and r vector (departure first)
void calc_itin_v_vectors_from_dates_and_r(struct ItinStep *step, CelestSystem *system);

// copy the body reference, r and v vectors and date from orig_step to step_copy
void copy_step_body_vectors_and_date(struct ItinStep *orig_step, struct ItinStep *step_copy);

// create and return copy of itineraries from given step (rising date)
struct ItinStep * create_itin_copy(struct ItinStep *step);

// create and return copy of single itinerary from arrival (falling date)
struct ItinStep * create_itin_copy_from_arrival(struct ItinStep *step);

// returns 1 if itinerary is valid and 0 if not (arrival first)
int is_valid_itinerary(struct ItinStep *step);

// store itineraries in text file from multiple departures (pre-order storing)
void store_itineraries_in_file(struct ItinStep **departures, int num_nodes, int num_deps);

void itinerary_short_overview_to_string(struct ItinStep *step, enum DateType date_type, double dep_periapsis, double arr_periapsis, char *string);

void itinerary_detailed_overview_to_string(struct ItinStep *step, enum DateType date_type, double dep_periapsis, double arr_periapsis, char *string);

void itinerary_step_parameters_to_string(char *s_label, char *s_values, enum DateType date, double dep_periapsis, double arr_periapsis, struct ItinStep *step);

// removes this and all now unneeded steps from itineraries (no next node before arrival)
void remove_step_from_itinerary(struct ItinStep *step);

// free to itinerary allocated memory
void free_itinerary(struct ItinStep *step);



#endif //KSP_ITIN_TOOL_H
