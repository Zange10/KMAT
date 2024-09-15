#ifndef KSP_DATABASE_H
#define KSP_DATABASE_H

#include <sqlite3.h>

enum VehicleStatus_DB {ACTIVE, INACTIVE};


// Mathematical Planes and Aeroplanes have for some reason the same name...
struct PlaneInfo_DB {
	int id;
	char name[30];
	int rated_alt;
	double F_vac;
	double F_sl;
	double burnrate;
	double mass_dry;
	double mass_wet;
	enum VehicleStatus_DB status;
};


void init_db();
int execute_query(const char *query);
sqlite3_stmt * execute_single_row_request(const char *query);
sqlite3_stmt * execute_multirow_request(const char *query);
struct PlaneInfo_DB db_get_plane_from_id(int id);
struct MissionProgram_DB db_get_program_from_id(int id);
void close_db();

#endif //KSP_DATABASE_H
