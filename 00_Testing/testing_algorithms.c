#include "testing_algorithms.h"
#include "tools/tool_funcs.h"
#include "orbit_calculator/transfer_calc.h"
#include "tools/file_io.h"
#include <string.h>
#include <stdlib.h>

int has_porkchop_point_valid_dep_arr_flyby_bodies(struct PorkchopPoint porkchop_point, struct ItinSequenceInfoToTarget seq_info_to_target) {
	int has_valid_fly_by_bodies = 1;
	struct ItinStep *step_ptr = porkchop_point.arrival;

	if(step_ptr->body != seq_info_to_target.arr_body) {has_valid_fly_by_bodies = 0; printf("Wrong Arrival Body: %s | %s\n", step_ptr->body->name, seq_info_to_target.arr_body->name);}
	if(get_first(step_ptr)->body != seq_info_to_target.dep_body) {has_valid_fly_by_bodies = 0; printf("Wrong Departure Body: %s | %s\n", get_first(step_ptr)->body->name, seq_info_to_target.dep_body->name);}

	while(step_ptr->prev != NULL && step_ptr->prev->prev != NULL) {
		step_ptr = step_ptr->prev;
		int is_part_of_flyby_bodies = 0;
		for(int i = 0; i < seq_info_to_target.num_flyby_bodies; i++) {
			if(step_ptr->body == seq_info_to_target.flyby_bodies[i]) {is_part_of_flyby_bodies = 1; break;}
		}
		if(!is_part_of_flyby_bodies) {has_valid_fly_by_bodies = 0; printf("%s is not part of the allowed fly-by bodies\n", step_ptr->body->name); break;}
	}
	return has_valid_fly_by_bodies;
}

int has_porkchop_point_valid_itinerary_sequence(struct PorkchopPoint porkchop_point, struct ItinSequenceInfoSpecItin seq_info_spec_itin) {
	int has_valid_seq = 1;
	struct ItinStep *step_ptr = porkchop_point.arrival;

	int step = seq_info_spec_itin.num_steps-1;

	while(step_ptr != NULL) {
		if(step_ptr->body != seq_info_spec_itin.bodies[step]) {has_valid_seq = 0; printf("Invalid itinerary sequence (Step: %d, Expected: %s, Instead: %s)\n", step, seq_info_spec_itin.bodies[step]->name, step_ptr->body->name); break;}
		step_ptr = step_ptr->prev;
		step--;
	}
	return has_valid_seq;
}

int is_porkchop_point_inside_time_constraints(struct PorkchopPoint porkchop_point, struct Itin_Calc_Data itin_calc_data) {
	int is_inside_time_constraints = 1;
	struct ItinStep *arrival = porkchop_point.arrival;
	if(arrival->date > itin_calc_data.jd_max_arr) {is_inside_time_constraints = 0; printf("Arrival after max arrival date: %f | %f\n", arrival->date, itin_calc_data.jd_max_arr);}
	if(get_first(arrival)->date < itin_calc_data.jd_min_dep) {is_inside_time_constraints = 0; printf("Departure before min departure date: %f | %f\n", get_first(arrival)->date, itin_calc_data.jd_min_dep);}
	if(get_first(arrival)->date > itin_calc_data.jd_max_dep) {is_inside_time_constraints = 0; printf("Departure after max departure date: %f | %f\n", get_first(arrival)->date, itin_calc_data.jd_max_dep);}
	return is_inside_time_constraints;
}

int is_porkchop_point_inside_dv_filter(struct PorkchopPoint porkchop_point, struct Dv_Filter dv_filter) {
	int is_inside_dv_filter = 1;
	double dv_sat = porkchop_point.dv_dsm;
	dv_sat += dv_filter.last_transfer_type == TF_CIRC ? porkchop_point.dv_arr_circ : dv_filter.last_transfer_type == TF_CAPTURE ? porkchop_point.dv_arr_cap : 0;
	if(porkchop_point.dv_dep > dv_filter.max_depdv) {is_inside_dv_filter = 0; printf("Departure dv too high: %f | %f\n", porkchop_point.dv_dep, dv_filter.max_depdv);}
	if(dv_sat > dv_filter.max_satdv) {is_inside_dv_filter = 0; printf("Satellite dv too high: %f | %f\n", dv_sat, dv_filter.max_satdv);}
	if(porkchop_point.dv_dep + dv_sat > dv_filter.max_totdv) {is_inside_dv_filter = 0; printf("Total dv too high: %f | %f\n", porkchop_point.dv_dep + dv_sat, dv_filter.max_totdv);}
	return is_inside_dv_filter;
}

int has_porkchop_point_valid_itinerary(struct PorkchopPoint porkchop_point) {
	int is_valid_itinerary = 1;
	struct ItinStep *step_ptr = porkchop_point.arrival;
	struct ItinStep *prev = porkchop_point.arrival->prev;

	while(prev->prev != NULL && prev->body != NULL) {
		double t[3] = {prev->prev->date, prev->date, step_ptr->date};
		Body *bodies[3] = {prev->prev->body, prev->body, step_ptr->body};
		if(!is_flyby_viable(prev->v_arr, step_ptr->v_dep, prev->v_body, prev->body, 10)) {
			is_valid_itinerary = 0;
			printf("Invalid Itinerary:\n%f - %s\n%f - %s\n%f - %s\n", t[0], bodies[0]->name, t[1], bodies[1]->name, t[2], bodies[2]->name);
			for(int i = 0; i < 3; i++) {
				print_date(convert_JD_date(t[i], DATE_ISO), 0); printf(" | "); print_date(convert_JD_date(t[i], DATE_KERBAL), 0); printf(" | ");
				printf("%s\n", bodies[i]->name);
			}
			break;
		}
		step_ptr = prev;
		prev = prev->prev;
	}

	return is_valid_itinerary;
}




// TEST METHODS -----------------------------------------------------------------------------------------------------------------------------------



enum TestResult test_itinerary_calculator(struct Itin_To_Target_Calc_Test test_data) {
	CelestSystem *system = get_system_by_name(test_data.system_name);
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
			.dep_periapsis = get_body_by_name(test_data.dep_body_name, system)->atmo_alt+50e3,
			.arr_periapsis = get_body_by_name(test_data.arr_body_name, system)->atmo_alt+50e3,
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
		seq_info_to_target.flyby_bodies = malloc(test_data.num_flyby_bodies * sizeof(Body*));
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

	enum TestResult test_result = TEST_PASSED;
	struct PorkchopPoint *porkchop_points = create_porkchop_array_from_departures(results.departures, results.num_deps, dv_filter.dep_periapsis, dv_filter.arr_periapsis);
	for(int i = 0; i < num_itins; i++) {
		if(!has_porkchop_point_valid_dep_arr_flyby_bodies(porkchop_points[i], seq_info_to_target)) {test_result = TEST_FAIL_ITINERARY_HAS_INVALID_CELESTIAL_BODY; break;}
		if(!is_porkchop_point_inside_time_constraints(porkchop_points[i], itin_calc_data)) {test_result = TEST_FAIL_ITINERARY_OUTSIDE_TIME_CONSTRAINTS; break;}
		if(!is_porkchop_point_inside_dv_filter(porkchop_points[i], dv_filter)) {test_result = TEST_FAIL_ITINERARY_NOT_INSIDE_DV_FILTER; break;}
		if(!has_porkchop_point_valid_itinerary(porkchop_points[i])) {test_result = TEST_FAIL_INVALID_ITINERARY; break;}
	}

	// FREE
	free(porkchop_points);
	for(int i = 0; i < results.num_deps; i++) free_itinerary(results.departures[i]);
	free(results.departures);
	if(test_data.num_flyby_bodies > 0) free(seq_info_to_target.flyby_bodies);

	if(test_result != TEST_PASSED) return test_result;

	printf("Number of Nodes: % 6d | Number of Itineraries: % 6d\n", results.num_nodes, num_itins);
	printf("Number of Nodes: % 6d | Number of Itineraries: % 6d  (EXPECTED)\n", test_data.expected_num_nodes, test_data.expected_num_itins);
	if(results.num_nodes != test_data.expected_num_nodes || num_itins != test_data.expected_num_itins) return TEST_WARN_DIFF_EXPECTED_NUM_NODES_OR_ITINS;

	printf("SUCCESS\n");

	return TEST_PASSED;
}

enum TestResult test_sequence_calculator(struct Itin_Spec_Seq_Calc_Test test_data) {
	CelestSystem *system = get_system_by_name(test_data.system_name);
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
			.dep_periapsis = get_body_by_name(test_data.body_names[0], system)->atmo_alt+50e3,
			.arr_periapsis = get_body_by_name(test_data.body_names[test_data.num_steps-1], system)->atmo_alt+50e3,
			.last_transfer_type = test_data.last_transfer_type
	};

	struct ItinSequenceInfoSpecItin seq_info_spec_seq = {
			.type = ITIN_SEQ_INFO_SPEC_SEQ,
			.system = system,
			.bodies = malloc(test_data.num_steps * sizeof(Body*)),
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

	enum TestResult test_result = TEST_PASSED;
	struct PorkchopPoint *porkchop_points = create_porkchop_array_from_departures(results.departures, results.num_deps, dv_filter.dep_periapsis, dv_filter.arr_periapsis);
	for(int i = 0; i < num_itins; i++) {
		if(!has_porkchop_point_valid_itinerary_sequence(porkchop_points[i], seq_info_spec_seq)) {test_result = TEST_FAIL_ITINERARY_HAS_INVALID_CELESTIAL_BODY; break;}
		if(!is_porkchop_point_inside_time_constraints(porkchop_points[i], itin_calc_data)) {test_result = TEST_FAIL_ITINERARY_OUTSIDE_TIME_CONSTRAINTS; break;}
		if(!is_porkchop_point_inside_dv_filter(porkchop_points[i], dv_filter)) {test_result = TEST_FAIL_ITINERARY_NOT_INSIDE_DV_FILTER; break;}
		if(!has_porkchop_point_valid_itinerary(porkchop_points[i])) {test_result = TEST_FAIL_INVALID_ITINERARY; break;}
	}

	// FREE
	free(porkchop_points);
	for(int i = 0; i < results.num_deps; i++) free_itinerary(results.departures[i]);
	free(results.departures);
	free(seq_info_spec_seq.bodies);

	if(test_result != TEST_PASSED) return test_result;

	printf("Number of Nodes: % 6d | Number of Itineraries: % 6d\n", results.num_nodes, num_itins);
	printf("Number of Nodes: % 6d | Number of Itineraries: % 6d  (EXPECTED)\n", test_data.expected_num_nodes, test_data.expected_num_itins);
	if(results.num_nodes != test_data.expected_num_nodes || num_itins != test_data.expected_num_itins) return TEST_WARN_DIFF_EXPECTED_NUM_NODES_OR_ITINS;

	printf("SUCCESS\n");

	return TEST_PASSED;
}

enum TestResult test_itins_file(char *filepath) {
	printf("Loading : %s\n", filepath);
	struct ItinsLoadFileResults file_results = load_itineraries_from_bfile(filepath);

	CelestSystem *system = file_results.header.system;
	struct ItinStep **departures = file_results.departures;
	int num_deps = file_results.header.num_deps;

	if(file_results.header.file_type > 2) {
		if(file_results.header.calc_data.seq_info.to_target.type == ITIN_SEQ_INFO_TO_TARGET) free(file_results.header.calc_data.seq_info.to_target.flyby_bodies);
		if(file_results.header.calc_data.seq_info.spec_seq.type == ITIN_SEQ_INFO_SPEC_SEQ) free(file_results.header.calc_data.seq_info.spec_seq.bodies);
	}

	printf("System: %s\nNumber of Departures: %d\n", system->name, num_deps);

	int num_itins = 0;
	for(int i = 0; i < num_deps; i++) num_itins += get_number_of_itineraries(departures[i]);

	printf("Number of Itineraries: %d\n", num_itins);

	enum TestResult test_result = TEST_PASSED;
	struct PorkchopPoint *porkchop_points = create_porkchop_array_from_departures(departures, num_deps, get_first(departures[0])->body->atmo_alt+50e3, get_last(departures[0])->body->atmo_alt+50e3);
	for(int i = 0; i < num_itins; i++) {
		if(!has_porkchop_point_valid_itinerary(porkchop_points[i])) {test_result = TEST_FAIL_INVALID_ITINERARY; break;}
	}


	// FREE
	free(porkchop_points);
	for(int i = 0; i < num_deps; i++) free_itinerary(departures[i]);
	free(departures);
	free_celestial_system(system);

	if(test_result == TEST_PASSED) printf("SUCCESS\n");

	return test_result;
}


enum TestResult test_itin_file(char *filepath) {
	printf("Loading : %s\n", filepath);
	struct ItinLoadFileResults file_results = load_single_itinerary_from_bfile(filepath);

	CelestSystem *system = file_results.system;
	struct ItinStep *arrival = file_results.itin;

	printf("System: %s\n", system->name);
	printf("Itinerary: ");
	struct ItinStep *ptr = get_first(arrival);
	while(ptr != NULL) {
		printf("%s", ptr->body->name);
		if(ptr->num_next_nodes > 0) {
			printf(" -> ");
			ptr = ptr->next[0];
		} else ptr = NULL;
	}
	printf("\n");

	enum TestResult test_result = TEST_PASSED;
	struct PorkchopPoint porkchop_point = create_porkchop_point(arrival, get_first(ptr)->body->atmo_alt+50e3, get_last(ptr)->body->atmo_alt+50e3);

	printf("DV: (%f | %f | %f | %f)\nDeparture date: ", porkchop_point.dv_dep, porkchop_point.dv_dsm, porkchop_point.dv_arr_cap, porkchop_point.dv_arr_circ);
	print_date(convert_JD_date(porkchop_point.dep_date, DATE_ISO), 0); printf("  |  "); print_date(convert_JD_date(porkchop_point.dep_date, DATE_KERBAL), 1);
	printf("Duration: %f days\n", porkchop_point.dur);

	if(!has_porkchop_point_valid_itinerary(porkchop_point)) test_result = TEST_FAIL_INVALID_ITINERARY;

	// FREE
	free_itinerary(arrival);
	free_celestial_system(system);

	if(test_result == TEST_PASSED) printf("SUCCESS\n");

	return test_result;
}