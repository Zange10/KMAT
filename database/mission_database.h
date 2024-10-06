#ifndef KSP_MISSION_DATABASE_H
#define KSP_MISSION_DATABASE_H


enum MissionStatus_DB {YET_TO_FLY, IN_PROGRESS, ENDED};
enum MissionSuccess {MISSION_SUCCESS, MISSION_PARTIAL_SUCCESS, MISSION_FAIL, MISSION_SUCCESS_TBD};
enum MissionObjectiveRank {OBJ_PRIMARY, OBJ_SECONDARY};
enum MissionObjectiveStatus {OBJ_TBD, OBJ_SUCCESS, OBJ_FAIL};

struct Mission_DB {
	int id;
	char name[30];
	int program_id;
	enum MissionStatus_DB status;
	int launcher_id;
	int plane_id;
};

struct Mission_Filter {
	char name[30];
	int program_id;
	int ytf, in_prog, ended;
};

struct MissionProgram_DB {
	int id;
	char name[30];
	char vision[255];
};

struct MissionObjective_DB {
	int id, mission_id;
	enum MissionObjectiveRank rank;
	char objective[500];
	char notes[500];
	enum MissionObjectiveStatus status;
};

struct MissionEvent_DB {
	int id, mission_id;
	double epoch;
	char event[500];
};

void db_new_program(const char *program_name, const char *vision);
void db_new_mission(const char *mission_name, int program_id, int launcher_id, int status);
void db_update_mission(int mission_id, const char *mission_name, int program_id, int launcher_id, int status);
void db_new_objective(int mission_id, int status, int rank, const char *objective);
void db_update_objective(int objective_id, int mission_id, int status, int rank, const char *objective);
void db_remove_objective(int objective_id);
void db_new_event(int mission_id, double epoch, char *event);
void db_update_event(int event_id, int mission_id, double epoch, char *event);
void db_remove_event(int event_id);
int db_get_number_of_programs();
int db_get_missions_ordered_by_launch_date(struct Mission_DB **p_missions, struct Mission_Filter filter);
int db_get_all_programs(struct MissionProgram_DB **p_programs);
struct MissionProgram_DB db_get_program_from_id(int id);
enum MissionSuccess db_get_mission_success(int mission_id);
int db_get_objectives_from_mission_id(struct MissionObjective_DB **p_objectives, int mission_id);
int db_get_events_from_mission_id(struct MissionEvent_DB **p_events, int mission_id);
int db_get_last_inserted_id();


// UNUSED
int is_initial_event(const char *event);

#endif //KSP_MISSION_DATABASE_H
