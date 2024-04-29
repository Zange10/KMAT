#ifndef KSP_DATABASE_H
#define KSP_DATABASE_H

#include <sqlite3.h>

enum MissionStatus_DB {YET_TO_FLY, IN_PROGRESS, ENDED};

enum VehicleStatus_DB {ACTIVE, INACTIVE};

#define MISSION_COLS 5	// one less than table has columns because plane and lv get in one

struct Mission_DB {
	int id;
	char name[30];
	int program_id;
	enum MissionStatus_DB status;
	int launcher_id;
	int plane_id;
};

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

struct MissionProgram_DB {
	int id;
	char name[30];
	char vision[255];
};


void init_db();
sqlite3_stmt * execute_single_row_request(const char *query);
sqlite3_stmt * execute_multirow_request(const char *query);
int db_get_missions(struct Mission_DB **missions);
struct PlaneInfo_DB db_get_plane_from_id(int id);
struct MissionProgram_DB db_get_program_from_id(int id);
void close_db();

#endif //KSP_DATABASE_H
