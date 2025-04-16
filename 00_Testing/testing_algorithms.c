#include "testing_algorithms.h"
#include "tools/tool_funcs.h"
#include "orbit_calculator/transfer_calc.h"
#include <string.h>
#include <stdlib.h>


struct System * get_system_by_name(char *name) {
	for(int i = 0; i < get_num_available_systems(); i++) {
		if(strcmp(get_available_systems()[i]->name, name) == 0) return get_available_systems()[i];
	}

	return NULL;
}

struct Body * get_body_by_name(char *name, struct System *system) {
	for(int i = 0; i < system->num_bodies; i++) {
		if(strcmp(system->bodies[i]->name, name) == 0) return system->bodies[i];
	}

	return NULL;
}

enum TestingResult test_itinerary_calculator(struct Itin_To_Target_Calc_Test test_data) {
	struct System *system = get_system_by_name(test_data.system_name);
	if(system == NULL) {
		printf("Celestial System not found: %s\n", test_data.system_name);
		return TEST_FAIL_CELESTIAL_SYSTEM_NOT_FOUND;
	}

	int max_duration = test_data.max_duration;
	if(test_data.min_dep_date.date_type == DATE_KERBAL) max_duration /= 4;
	struct Dv_Filter dv_filter = {
			.max_depdv = test_data.max_depdv,
			.max_satdv = test_data.max_satdv,
			.max_totdv = test_data.max_totdv,
			.last_transfer_type = test_data.last_transfer_type
	};

	struct ItinSequenceInfoToTarget seq_info_to_target = {
			.type = ITIN_SEQ_INFO_TO_TARGET,
			.system = system,
			.num_flyby_bodies = test_data.num_flyby_bodies > 0 ? test_data.num_flyby_bodies : system->num_bodies,
			.flyby_bodies = test_data.num_flyby_bodies > 0 ? NULL : system->bodies,
			.dep_body = get_body_by_name(test_data.dep_body_name, system),
			.arr_body = get_body_by_name(test_data.arr_body_name, system)
	};

	if(seq_info_to_target.dep_body == NULL || seq_info_to_target.arr_body == NULL) {
		printf("Celestial Body not found: %s or %s\n", test_data.dep_body_name, test_data.arr_body_name);
		return TEST_FAIL_CELESTIAL_BODY_NOT_FOUND;
	}
	if(test_data.num_flyby_bodies > 0) {
		seq_info_to_target.flyby_bodies = malloc(test_data.num_flyby_bodies * sizeof(struct Body*));
		for(int i = 0; i < test_data.num_flyby_bodies; i++) {
			seq_info_to_target.flyby_bodies[i] = get_body_by_name(test_data.flyby_bodies_names[i], system);
			if(seq_info_to_target.flyby_bodies[i] == NULL) {
				free(seq_info_to_target.flyby_bodies);
				printf("Celestial Body not found: %s\n", test_data.flyby_bodies_names[i]);
				return TEST_FAIL_CELESTIAL_BODY_NOT_FOUND;
			}
		}
	}

	ItinSequenceInfo seq_info;
	seq_info.to_target = seq_info_to_target;

	struct Itin_Calc_Data itin_calc_data = {
			.jd_min_dep = convert_date_JD(test_data.min_dep_date),
			.jd_max_dep = convert_date_JD(test_data.max_dep_date),
			.jd_max_arr = convert_date_JD(test_data.max_arr_date),
			.max_duration = max_duration,
			.dv_filter = dv_filter,
			.seq_info = seq_info
	};

	struct Itin_Calc_Results results = search_for_itineraries(itin_calc_data);

	int num_itins = 0;
	for(int i = 0; i < results.num_deps; i++) num_itins += get_number_of_itineraries(results.departures[i]);

	for(int i = 0; i < results.num_deps; i++) free_itinerary(results.departures[i]);
	free(results.departures);
	if(test_data.num_flyby_bodies > 0) free(seq_info_to_target.flyby_bodies);

	printf("Number of Nodes: % 6d | Number of Itineraries: % 6d\n", results.num_nodes, num_itins);
	printf("Number of Nodes: % 6d | Number of Itineraries: % 6d  (EXPECTED)\n", test_data.expected_num_nodes, test_data.expected_num_itins);
	if(results.num_nodes != test_data.expected_num_nodes || num_itins != test_data.expected_num_itins) return TEST_WARN_DIFF_EXPECTED_NUM_NODES_OR_ITINS;

	return TEST_PASSED;
}

enum TestingResult test_sequence_calculator(struct Itin_Spec_Seq_Calc_Test test_data) {
	struct System *system = get_system_by_name(test_data.system_name);
	if(system == NULL) {
		printf("Celestial System not found: %s\n", test_data.system_name);
		return TEST_FAIL_CELESTIAL_SYSTEM_NOT_FOUND;
	}

	int max_duration = test_data.max_duration;
	if(test_data.min_dep_date.date_type == DATE_KERBAL) max_duration /= 4;
	struct Dv_Filter dv_filter = {
			.max_depdv = test_data.max_depdv,
			.max_satdv = test_data.max_satdv,
			.max_totdv = test_data.max_totdv,
			.last_transfer_type = test_data.last_transfer_type
	};

	struct ItinSequenceInfoSpecItin seq_info_spec_seq = {
			.type = ITIN_SEQ_INFO_SPEC_SEQ,
			.system = system,
			.bodies = malloc(test_data.num_steps * sizeof(struct Body*)),
			.num_steps = test_data.num_steps
	};

	if(test_data.num_steps > 1) {
		for(int i = 0; i < test_data.num_steps; i++) {
			seq_info_spec_seq.bodies[i] = get_body_by_name(test_data.body_names[i], system);
			if(seq_info_spec_seq.bodies[i] == NULL) {
				free(seq_info_spec_seq.bodies);
				printf("Celestial Body not found: %s\n", test_data.body_names[i]);
				return TEST_FAIL_CELESTIAL_BODY_NOT_FOUND;
			}
		}
	} else {
		printf("Number of steps is lower than 2\n");
		free(seq_info_spec_seq.bodies); return TEST_FAIL_CONFIGURATION_ERROR;
	}

	ItinSequenceInfo seq_info;
	seq_info.spec_seq = seq_info_spec_seq;

	struct Itin_Calc_Data itin_calc_data = {
			.jd_min_dep = convert_date_JD(test_data.min_dep_date),
			.jd_max_dep = convert_date_JD(test_data.max_dep_date),
			.jd_max_arr = convert_date_JD(test_data.max_arr_date),
			.max_duration = max_duration,
			.dv_filter = dv_filter,
			.seq_info = seq_info
	};

	struct Itin_Calc_Results results = search_for_itineraries(itin_calc_data);

	int num_itins = 0;
	for(int i = 0; i < results.num_deps; i++) num_itins += get_number_of_itineraries(results.departures[i]);

	for(int i = 0; i < results.num_deps; i++) free_itinerary(results.departures[i]);
	free(results.departures);
	free(seq_info_spec_seq.bodies);

	printf("Number of Nodes: % 6d | Number of Itineraries: % 6d\n", results.num_nodes, num_itins);
	printf("Number of Nodes: % 6d | Number of Itineraries: % 6d  (EXPECTED)\n", test_data.expected_num_nodes, test_data.expected_num_itins);
	if(results.num_nodes != test_data.expected_num_nodes || num_itins != test_data.expected_num_itins) return TEST_WARN_DIFF_EXPECTED_NUM_NODES_OR_ITINS;

	return TEST_PASSED;
}
