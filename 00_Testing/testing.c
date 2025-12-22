#include <gtk/gtk.h>
#include "testing.h"
#include "testing_algorithms.h"
#include "tools/tool_funcs.h"
#include "tools/file_io.h"


void run_tests() {
	int num_tests = 0;
	const int max_num_tests = 1000;
	union TestData test_data[max_num_tests];


	// ITINERARY CALCULATOR TEST DATA #################################################################################

	// Kerbin -> Eeloo; 1-001 | 2-001 | 3000; dv_dep = 1500 | Bodies: all
	test_data[num_tests] = (union TestData) (struct Itin_To_Target_Calc_Test) {
			TEST_ITIN_TO_TARGET,
			(struct Datetime) {.y = 1, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 2, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 100, .d = 1, .date_type = DATE_KERBAL},
			3000,
			1500, 1e9, 1e9, TF_FLYBY,
			"Stock System",
			-1,
			(char *[]){""},
			"KERBIN",
			"EELOO",
			1321, 456 // expected num_nodes and num_itins
	}; num_tests++;

	// Kerbin -> Eeloo; 1-001 | 2-001 | 3000; dv_dep = 1800, dv_arr = 1800 (circ) | Bodies: Eve, Kerbin, Duna, Eeloo
	test_data[num_tests] = (union TestData) (struct Itin_To_Target_Calc_Test) {
			TEST_ITIN_TO_TARGET,
			(struct Datetime) {.y = 1, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 2, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 100, .d = 1, .date_type = DATE_KERBAL},
			3000,
			1800, 1800, 1e9, TF_CIRC,
			"Stock System",
			4,
			(char *[]){"EVE", "KERBIN", "DUNA", "EELOO"},
			"KERBIN",
			"EELOO",
			811, 266 // expected num_nodes and num_itins
	}; num_tests++;

	// Kerbin -> Kerbin; 1-001 | 2-001 | 1000; dv_dep = 1800 | Bodies: Eve, Kerbin, Duna, Jool
	test_data[num_tests] = (union TestData) (struct Itin_To_Target_Calc_Test) {
			TEST_ITIN_TO_TARGET,
			(struct Datetime) {.y = 1, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 2, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 100, .d = 1, .date_type = DATE_KERBAL},
			1000,
			1800, 1e9, 1e9, TF_FLYBY,
			"Stock System",
			4,
			(char *[]){"EVE", "KERBIN", "DUNA", "JOOL"},
			"KERBIN",
			"KERBIN",
			7760, 4023 // expected num_nodes and num_itins
	}; num_tests++;

	// Earth -> Earth; 1959-01-01 | 1960-01-01 | 1000; dv_dep = 5000 | Bodies: all
	test_data[num_tests] = (union TestData) (struct Itin_To_Target_Calc_Test) {
			TEST_ITIN_TO_TARGET,
			(struct Datetime) {.y = 1959, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 1960, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 2000, .m = 1, .d = 1, .date_type = DATE_ISO},
			1000,
			5000, 1e9, 1e9, TF_FLYBY,
			"Solar System (Ephemerides)",
			-1,
			(char *[]){""},
			"EARTH",
			"EARTH",
			100166, 51288 // expected num_nodes and num_itins
	}; num_tests++;

	// Earth -> Earth; 1959-01-01 | 1960-01-01 | 1000; dv_dep = 5000 | Bodies: Mercury, Venus, Earth
	test_data[num_tests] = (union TestData) (struct Itin_To_Target_Calc_Test) {
			TEST_ITIN_TO_TARGET,
			(struct Datetime) {.y = 1959, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 1960, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 2000, .m = 1, .d = 1, .date_type = DATE_ISO},
			1000,
			5000, 1e9, 1e9, TF_FLYBY,
			"Solar System (Ephemerides)",
			3,
			(char *[]){"MERCURY", "VENUS", "EARTH"},
			"EARTH",
			"EARTH",
			80937, 42056 // expected num_nodes and num_itins
	}; num_tests++;

	// Earth -> Jupiter; 1967-01-01 | 1968-01-01 | 3000; dv_dep = 4500 | Bodies: Venus, Earth, Mars, Jupiter
	test_data[num_tests] = (union TestData) (struct Itin_To_Target_Calc_Test) {
			TEST_ITIN_TO_TARGET,
			(struct Datetime) {.y = 1967, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 1968, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 2000, .m = 1, .d = 1, .date_type = DATE_ISO},
			3000,
			4500, 1e9, 1e9, TF_FLYBY,
			"Solar System (Ephemerides)",
			4,
			(char *[]){"VENUS", "EARTH", "MARS", "JUPITER"},
			"EARTH",
			"JUPITER",
			18649, 3881 // expected num_nodes and num_itins
	}; num_tests++;


	// SEQUENCE CALCULATOR TEST DATA #################################################################################

	// Kerbin -> Eve; 2-001 | 3-001 | 300; dv_dep = 1200; dv_arr = 1500 (circ)
	test_data[num_tests] = (union TestData) (struct Itin_Spec_Seq_Calc_Test) {
			TEST_ITIN_SPEC_SEQ,
			(struct Datetime) {.y = 2, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 3, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 100, .d = 1, .date_type = DATE_KERBAL},
			300,
			1200, 1500, 1e9, TF_CIRC,
			"Stock System",
			2,
			(char *[]){"KERBIN", "EVE"},
			1568, 1541 // expected num_nodes and num_itins
	}; num_tests++;

	// Kerbin -> Eve -> Duna -> Kerbin; 2-001 | 3-001 | 1000; dv_dep = 1500
	test_data[num_tests] = (union TestData) (struct Itin_Spec_Seq_Calc_Test) {
			TEST_ITIN_SPEC_SEQ,
			(struct Datetime) {.y = 2, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 3, .d = 1, .date_type = DATE_KERBAL},
			(struct Datetime) {.y = 100, .d = 1, .date_type = DATE_KERBAL},
			1000,
			1500, 1e9, 1e9, TF_FLYBY,
			"Stock System",
			4,
			(char *[]){"KERBIN", "EVE", "DUNA", "KERBIN"},
			1813, 599 // expected num_nodes and num_itins
	}; num_tests++;

	// Earth -> Mars; 1950-01-01 | 1951-06-01 | 1950-11-01 | 600; dv_dep = 5000; dv_arr = 4000 (circ); dv_tot = 8200
	test_data[num_tests] = (union TestData) (struct Itin_Spec_Seq_Calc_Test) {
			TEST_ITIN_SPEC_SEQ,
			(struct Datetime) {.y = 1950, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 1950, .m = 6, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 1950, .m =11, .d = 1, .date_type = DATE_ISO},
			600,
			5000, 4000, 8200, TF_CIRC,
			"Solar System (Ephemerides)",
			2,
			(char *[]){"EARTH", "MARS"},
			1984, 1946 // expected num_nodes and num_itins
	}; num_tests++;

	// Earth -> Jupiter -> Saturn -> Uranus -> Neptune; 1977-01-01 | 1978-01-01 | 8000; dv_dep = 7200
	test_data[num_tests] = (union TestData) (struct Itin_Spec_Seq_Calc_Test) {
			TEST_ITIN_SPEC_SEQ,
			(struct Datetime) {.y = 1977, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 1978, .m = 1, .d = 1, .date_type = DATE_ISO},
			(struct Datetime) {.y = 2000, .m = 1, .d = 1, .date_type = DATE_ISO},
			8000,
			7200, 1e9, 1e9, TF_FLYBY,
			"Solar System (Ephemerides)",
			5,
			(char *[]){"EARTH", "JUPITER", "SATURN", "URANUS", "NEPTUNE"},
			5920, 1472 // expected num_nodes and num_itins
	}; num_tests++;



	// .ITINS-FILE TEST DATA #################################################################################

	char *itins_directory = "../00_Testing/Itineraries/";
	create_directory_if_not_exists(itins_directory);
	GDir *dir = g_dir_open(itins_directory, 0, NULL);
	if (!dir) {
		g_printerr("Unable to open directory: %s\n", itins_directory);
		return;
	}
	const gchar *filename;
	while ((filename = g_dir_read_name(dir)) != NULL) {
		if (g_str_has_suffix(filename, ".itins")) {
			char *filepath = malloc(100 * sizeof(char));
			sprintf(filepath, "%s%s", itins_directory, filename);
			test_data[num_tests] = (union TestData) (struct Itins_File_Test) {
					TEST_ITINS_FILE,
					filepath
			}; num_tests++;
		}
	}
	g_dir_close(dir);


	// .ITIN-FILE TEST DATA #################################################################################

	char *itin_directory = "../00_Testing/Itineraries/";
	create_directory_if_not_exists(itin_directory);
	dir = g_dir_open(itin_directory, 0, NULL);
	if (!dir) {
		g_printerr("Unable to open directory: %s\n", itin_directory);
		return;
	}
	while ((filename = g_dir_read_name(dir)) != NULL) {
		if (g_str_has_suffix(filename, ".itin")) {
			char *filepath = malloc(100 * sizeof(char));
			sprintf(filepath, "%s%s", itin_directory, filename);
			test_data[num_tests] = (union TestData) (struct Itin_File_Test) {
					TEST_ITIN_FILE,
					filepath
			}; num_tests++;
		}
	}




	// TEST RESULTS #################################################################################

	enum TestResult test_results[max_num_tests];

	for(int i = 10; i < num_tests; i++) {
		printf("###################### TEST %02d/%02d ######################\n", i+1, num_tests);
		switch(test_data[i].itin_to_target_test.test_type) {
			case TEST_ITIN_TO_TARGET:
				test_results[i] = test_itinerary_calculator(test_data[i].itin_to_target_test);
				break;
			case TEST_ITIN_SPEC_SEQ:
				test_results[i] = test_sequence_calculator(test_data[i].itin_spec_seq_calc_test);
				break;
			case TEST_ITINS_FILE:
				test_results[i] = test_itins_file(test_data[i].itins_file_test.filepath);
				free(test_data[i].itins_file_test.filepath);
				break;
			case TEST_ITIN_FILE:
				test_results[i] = test_itin_file(test_data[i].itin_file_test.filepath);
				free(test_data[i].itin_file_test.filepath);
				break;
			default:
				break;
		}
		printf("------------------------------------------------------------\n\n");
	}


	printf("\n\n##################### TEST RESULTS #####################\n");
	for(int i = 10; i < num_tests; i++) {
		printf("TEST %02d: ", i+1);
		switch(test_results[i]) {
			case TEST_PASSED:
				printf("PASS\n");
				break;
			case TEST_FAIL_CELESTIAL_SYSTEM_NOT_FOUND:
				printf("FAIL  |  Celestial system not found\n");
				break;
			case TEST_FAIL_CELESTIAL_BODY_NOT_FOUND:
				printf("FAIL  |  Celestial body not found\n");
				break;
			case TEST_FAIL_CONFIGURATION_ERROR:
				printf("FAIL  |  Configuration error\n");
				break;
			case TEST_WARN_DIFF_EXPECTED_NUM_NODES_OR_ITINS:
				printf("WARN  |  Number of nodes and/or number of itineraries differ from expectation\n");
				break;
			case TEST_FAIL_ITINERARY_NOT_INSIDE_DV_FILTER:
				printf("FAIL  |  Itinerary not inside dv-filter\n");
				break;
			case TEST_FAIL_ITINERARY_HAS_INVALID_CELESTIAL_BODY:
				printf("FAIL  |  Itinerary has invalid celestial body\n");
				break;
			case TEST_FAIL_ITINERARY_OUTSIDE_TIME_CONSTRAINTS:
				printf("FAIL  |  Itinerary is outside time constraint\n");
				break;
			case TEST_FAIL_INVALID_ITINERARY:
				printf("FAIL  |  Has invalid itinerary\n");
				break;
			default:
				printf("\n");
		}
	}
	printf("--------------------------------------------------------\n\n");

}