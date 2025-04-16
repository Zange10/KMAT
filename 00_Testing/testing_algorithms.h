#ifndef KSP_TESTING_ALGORITHMS_H
#define KSP_TESTING_ALGORITHMS_H


#include "tools/datetime.h"
#include "orbit_calculator/itin_tool.h"


enum TestType {TEST_ITIN_TO_TARGET, TEST_ITIN_SPEC_SEQ};

enum TestingResult {
	TEST_PASSED,
	TEST_FAIL_CELESTIAL_SYSTEM_NOT_FOUND,
	TEST_FAIL_CELESTIAL_BODY_NOT_FOUND,
	TEST_FAIL_CONFIGURATION_ERROR,
	TEST_WARN_DIFF_EXPECTED_NUM_NODES_OR_ITINS
};

union TestData { ;
	struct Itin_To_Target_Calc_Test {
		enum TestType test_type;
		struct Date min_dep_date, max_dep_date, max_arr_date;
		int max_duration;
		double max_depdv, max_satdv, max_totdv;
		enum LastTransferType last_transfer_type;
		char *system_name;
		int num_flyby_bodies;
		char **flyby_bodies_names;
		char *dep_body_name;
		char *arr_body_name;
		int expected_num_nodes, expected_num_itins;
	} itin_to_target_test;

	struct Itin_Spec_Seq_Calc_Test {
		enum TestType test_type;
		struct Date min_dep_date, max_dep_date, max_arr_date;
		int max_duration;
		double max_depdv, max_satdv, max_totdv;
		enum LastTransferType last_transfer_type;
		char *system_name;
		int num_steps;
		char **body_names;
		int expected_num_nodes, expected_num_itins;
	} itin_spec_seq_calc_test;
};

enum TestingResult test_itinerary_calculator(struct Itin_To_Target_Calc_Test test_data);

enum TestingResult test_sequence_calculator(struct Itin_Spec_Seq_Calc_Test test_data);



#endif //KSP_TESTING_ALGORITHMS_H
