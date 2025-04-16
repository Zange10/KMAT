#ifndef KSP_TRANSFER_CALC_H
#define KSP_TRANSFER_CALC_H

#include "tools/analytic_geometry.h"
#include "itin_tool.h"
#include "celestial_bodies.h"


typedef struct Itin_Calc_Data {
	double jd_min_dep;
	double jd_max_dep;
	double jd_max_arr;
	int max_duration;
	ItinSequenceInfo seq_info;
	struct Dv_Filter dv_filter;
} Itin_Calc_Data;

typedef struct Itin_Calc_Results {
	struct ItinStep **departures;
	int num_deps, num_nodes;
} Itin_Calc_Results;

typedef struct Transfer_Calc_Status {
	int num_itins;
	int num_deps;
	double jd_diff;
	double progress;
} Transfer_Calc_Status;

Itin_Calc_Results search_for_itineraries(Itin_Calc_Data calc_data);

Transfer_Calc_Status get_current_transfer_calc_status();

#endif //KSP_TRANSFER_CALC_H
