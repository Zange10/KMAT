#ifndef CSW_WRITER
#define CSW_WRITER


#include "celestial_systems.h"
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


typedef struct {
	int file_type;
	int num_nodes, num_deps, num_itins;
	CelestSystem *system;
	Itin_Calc_Data calc_data;
} ItinStepBinHeaderData;

struct ItinsLoadFileResults {
	ItinStepBinHeaderData header;
	struct ItinStep **departures;
};

struct ItinLoadFileResults {
	struct ItinStep *itin;
	CelestSystem *system;
};

// store itineraries in binary file from multiple departures (pre-order storing)
void store_itineraries_in_bfile(struct ItinStep **departures, int num_nodes, int num_deps, int num_itins, Itin_Calc_Data calc_data, CelestSystem *system, char *filepath, int file_type);

void print_header_data_to_string(ItinStepBinHeaderData header, char *string, enum DateType date_format);

// load itineraries from binary file for multiple departures (from pre-order storing)
struct ItinsLoadFileResults load_itineraries_from_bfile(char *filepath);

// stores single itinerary (first branches in tree) (departure first)
void store_single_itinerary_in_bfile(struct ItinStep *itin, CelestSystem *system, char *filepath);

// loads single itinerary (first branches in tree) (departure first)
struct ItinLoadFileResults load_single_itinerary_from_bfile(char *filepath);

// returns calc parameters used to create .itins-file
ItinStepBinHeaderData get_itins_bfile_header(FILE *file);


#endif
