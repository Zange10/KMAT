#ifndef KSP_TRANSFER_CALC_H
#define KSP_TRANSFER_CALC_H

#include "itin_tool.h"
#include "tools/celestial_systems.h"


typedef struct Itin_Calc_Data {
	double jd_min_dep;
	double jd_max_dep;
	double jd_max_arr;
	double max_duration;
	double step_dep_date;
	int num_deps_per_date;
	int max_num_waiting_orbits;
	struct Dv_Filter dv_filter;
	ItinSequenceInfo seq_info;
} Itin_Calc_Data;

typedef struct Itin_Calc_Results {
	struct ItinStep **departures;
	int num_deps, num_nodes, num_itins;
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
