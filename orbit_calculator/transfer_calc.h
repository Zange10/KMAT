#ifndef KSP_TRANSFER_CALC_H
#define KSP_TRANSFER_CALC_H

#include "tools/analytic_geometry.h"
#include "itin_tool.h"
#include "celestial_bodies.h"

struct Transfer_Spec_Calc_Data {
	struct Body **bodies;
	int num_steps;
	double jd_min_dep;
	double jd_max_dep;
	double jd_max_arr;
	int max_duration;
	struct System *system;
	struct Dv_Filter dv_filter;
};

struct Transfer_To_Target_Calc_Data {
	double jd_min_dep;
	double jd_max_dep;
	double jd_max_arr;
	int max_duration;
	struct ItinSequenceInfo *seq_info;
	struct Dv_Filter dv_filter;
};

struct Transfer_Calc_Results {
	struct ItinStep **departures;
	int num_deps, num_nodes;
};

struct Transfer_Calc_Status {
	int num_itins;
	int num_deps;
	double jd_diff;
	double progress;
};

struct Transfer_Calc_Results search_for_spec_itinerary(struct Transfer_Spec_Calc_Data calc_data);

struct Transfer_Calc_Results search_for_itinerary_to_target(struct Transfer_To_Target_Calc_Data calc_data);

struct Transfer_Calc_Status get_current_transfer_calc_status();

#endif //KSP_TRANSFER_CALC_H
