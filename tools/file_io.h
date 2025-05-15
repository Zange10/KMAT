#ifndef CSW_WRITER
#define CSW_WRITER


#include "celestial_bodies.h"
#include "orbit_calculator/itin_tool.h"
#include "orbit_calculator/transfer_calc.h"


int get_current_bin_file_type();


void create_directory_if_not_exists(const char *path);

/**
 * @brief Writes a .csv-file with fields and their data
 *
 * @param fields A string with the fields separated by commas
 * @param data The data to be stored in the .csv-file
 */
void write_csv(char fields[], double data[]);

/**
 * @brief Calculates the amount of given fields in given string
 *
 * @param fields A string with the fields separated by commas
 */
int amt_of_fields(char fields[]);


struct ItinsLoadFileResults {
	struct ItinStep **departures;
	struct System *system;
	int num_deps;
};

struct ItinLoadFileResults {
	struct ItinStep *itin;
	struct System *system;
};


void store_system_in_config_file(struct System *system);
struct System * load_system_from_config_file(char *filepath);

// store itineraries in binary file from multiple departures (pre-order storing)
void store_itineraries_in_bfile(struct ItinStep **departures, int num_nodes, int num_deps, Itin_Calc_Data calc_data, struct System *system, char *filepath, int file_type);

// load itineraries from binary file for multiple departures (from pre-order storing)
struct ItinsLoadFileResults load_itineraries_from_bfile(char *filepath);

// stores single itinerary (first branches in tree) (departure first)
void store_single_itinerary_in_bfile(struct ItinStep *itin, struct System *system, char *filepath);

// loads single itinerary (first branches in tree) (departure first)
struct ItinLoadFileResults load_single_itinerary_from_bfile(char *filepath);

#endif
