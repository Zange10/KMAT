#include "mission_database.h"
#include "database.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



const int NUM_INIT_EVENTS_DESIGNATORS = 3;
const char *INIT_EVENTS_DESIGNATORS[] = {
		"Launch",
		"Take-off",
		"Airlaunch"
};


void db_new_program(const char *program_name, const char *vision) {
	char query[500];
	sprintf(query, "INSERT INTO Program (Name, Vision) "
				   "VALUES ('%s', '%s');", program_name, vision);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! Program insert Error !!!!!!!!\n");
}

void db_new_mission(const char *mission_name, int program_id, int launcher_id, int status) {
	char query[500];
	sprintf(query, "INSERT INTO Mission (Name, ProgramID, LauncherID, Status) "
				   "VALUES ('%s', %d, %d, %d);", mission_name, program_id, launcher_id, status);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! Mission insert Error !!!!!!!!\n");
}

void db_update_mission(int mission_id, const char *mission_name, int program_id, int launcher_id, int status) {
	char query[500];
	sprintf(query, "UPDATE Mission "
				   "SET Name = '%s', ProgramID = %d, LauncherID = %d, Status = %d "
				   "WHERE MissionID = %d;", mission_name, program_id, launcher_id, status, mission_id);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! Mission update Error !!!!!!!!\n");
}

void db_new_objective(int mission_id, int status, int rank, const char *objective) {
	char query[500];
	sprintf(query, "INSERT INTO MissionObjective (MissionID, Status, ObjectiveRank, Objective) "
				   "VALUES (%d, %d, %d, '%s');", mission_id, status, rank, objective);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! MissionObjective insert Error !!!!!!!!\n");
}

void db_update_objective(int objective_id, int mission_id, int status, int rank, const char *objective) {
	char query[500];
	sprintf(query, "UPDATE MissionObjective "
				   "SET MissionId = %d, Status = %d, ObjectiveRank = %d, Objective = '%s' "
				   "WHERE ObjectiveID = %d;", mission_id, status, rank, objective, objective_id);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! MissionObjective update Error !!!!!!!!\n");
}

void db_remove_objective(int objective_id) {
	char query[500];
	sprintf(query, "DELETE FROM MissionObjective WHERE ObjectiveID = %d;", objective_id);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! MissionObjective delete Error !!!!!!!!\n");
}

void db_new_event(int mission_id, double epoch, char *event) {
	char query[500];
	sprintf(query, "INSERT INTO MissionEvent (MissionID, Time, Event) "
				   "VALUES (%d, datetime(%f), '%s');", mission_id, epoch, event);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! MissionObjective insert Error !!!!!!!!\n");
}

void db_update_event(int event_id, int mission_id, double epoch, char *event) {
	char query[500];
	sprintf(query, "UPDATE MissionEvent "
				   "SET MissionId = %d, Time = datetime(%f), Event = '%s' "
				   "WHERE EventID = %d;", mission_id, epoch, event, event_id);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! MissionObjective update Error !!!!!!!!\n");
}

void db_remove_event(int event_id) {
	char query[500];
	sprintf(query, "DELETE FROM MissionEvent WHERE EventID = %d;", event_id);
	if(execute_query(query) != SQLITE_OK) fprintf(stderr, "\n!!!!! MissionObjective delete Error !!!!!!!!\n");
}

int db_get_missions_ordered_by_launch_date(struct Mission_DB **p_missions, struct Mission_Filter filter) {
	char query[550];
	char string_programId[20];
	if(filter.program_id == 0) sprintf(string_programId, "");
	else sprintf(string_programId, "AND ProgramID = %d\n", filter.program_id);

	sprintf(query, "SELECT * FROM\n"
				   "    Mission m\n"
				   "LEFT JOIN (\n"
				   "    SELECT\n"
				   "        m.MissionID,\n"
				   "        MIN(me.Time) AS FirstEventDate\n"
				   "    FROM\n"
				   "        Mission m\n"
				   "    LEFT JOIN\n"
				   "        MissionEvent me\n"
				   "    ON\n"
				   "        m.MissionID = me.MissionID\n"
				   "    GROUP BY\n"
				   "        m.MissionID\n"
				   ") AS fe\n"
				   "ON\n"
				   "    m.MissionID = fe.MissionID\n"
				   "WHERE\n"
				   "    m.Name LIKE '%%%s%'\n"
				   "    AND (m.Status = %d OR m.Status = %d OR m.Status = %d)\n"
				   "	%s"
				   "ORDER BY\n"
				   "    CASE\n"
				   "        WHEN fe.FirstEventDate IS NULL THEN 1\n"
				   "        ELSE 0\n"
				   "    END,\n"
				   "    fe.FirstEventDate;", filter.name, filter.ytf?0:-1, filter.in_prog?1:-1, filter.ended?2:-1, string_programId);


	sqlite3_stmt *stmt = execute_multirow_request(query);
	int rc;

	int index = 0;
	int max_size = 4;
	struct Mission_DB *missions = (struct Mission_DB *) malloc(max_size*sizeof(struct Mission_DB));
	*p_missions = missions;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			const char *columnName = sqlite3_column_name(stmt, i);

			if 			(strcmp(columnName, "MissionID") == 0) {
				missions[index].id = sqlite3_column_int(stmt, i);
			} else if 	(strcmp(columnName, "ProgramID") == 0) {
				missions[index].program_id = sqlite3_column_int(stmt, i);
			} else if 	(strcmp(columnName, "Name") == 0) {
				sprintf(missions[index].name, (char *) sqlite3_column_text(stmt, i));
			} else if 	(strcmp(columnName, "Status") == 0) {
				int status = sqlite3_column_int(stmt, i);
				missions[index].status = status == 0 ? YET_TO_FLY : ((status == 1) ? IN_PROGRESS : ENDED);
			} else if 	(strcmp(columnName, "LauncherID") == 0) {
				missions[index].launcher_id = sqlite3_column_int(stmt, i);	// returns 0 if NULL
			} else if 	(strcmp(columnName, "PlaneID") == 0) {
				missions[index].plane_id = sqlite3_column_int(stmt, i);	// returns 0 if NULL
			} else {
				//fprintf(stderr, "!!!!!! %s not yet implemented for Mission !!!!!\n", columnName);
			}
		}

		index++;
		if(index >= max_size) {
			max_size *= 2;
			struct Mission_DB *temp = (struct Mission_DB *) realloc(missions, max_size*sizeof(struct Mission_DB));
			if(temp == NULL) {
				fprintf(stderr, "!!!!!! Reallocation failed for missions from database !!!!!!!!!!");
				return index;
			}
			missions = temp;
			*p_missions = missions;
		}
	}



	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Error stepping through the result set");
	}

	sqlite3_finalize(stmt);

	return index;
}

int db_get_number_of_programs() {
	char query[255];

	sprintf(query, "SELECT COUNT(*) FROM Program;");
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return 0;

	int num_profiles = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);

	return num_profiles;
}

int db_get_all_programs(struct MissionProgram_DB **p_programs) {
	char query[500];
	sprintf(query, "SELECT * FROM Program");


	sqlite3_stmt *stmt = execute_multirow_request(query);
	int rc;

	int index = 0;
	int max_size = 10;
	struct MissionProgram_DB *programs = (struct MissionProgram_DB *) malloc(max_size*sizeof(struct MissionProgram_DB));
	*p_programs = programs;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			const char *columnName = sqlite3_column_name(stmt, i);

			if 			(strcmp(columnName, "ProgramID") == 0) {
				programs[index].id = sqlite3_column_int(stmt, i);
			} else if 	(strcmp(columnName, "Name") == 0) {
				sprintf(programs[index].name, (char *) sqlite3_column_text(stmt, i));
			} else if 	(strcmp(columnName, "Vision") == 0) {
				sprintf(programs[index].vision, (char *) sqlite3_column_text(stmt, i));
			} else {
				fprintf(stderr, "!!!!!! %s not yet implemented for Mission !!!!!\n", columnName);
			}
		}
		index++;
		if(index >= max_size) {
			max_size *= 2;
			struct MissionProgram_DB *temp = (struct MissionProgram_DB *) realloc(programs, max_size*sizeof(struct MissionProgram_DB));
			if(temp == NULL) {
				fprintf(stderr, "!!!!!! Reallocation failed for missions from database !!!!!!!!!!");
				return index;
			}
			programs = temp;
			*p_programs = programs;
		}
	}

	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Error stepping through the result set");
	}

	sqlite3_finalize(stmt);

	return index;
}

struct MissionProgram_DB db_get_program_from_id(int id) {
	char query[48];
	sprintf(query, "SELECT * FROM Program WHERE ProgramID = %d", id);
	struct MissionProgram_DB program = {-1};

	sqlite3_stmt *stmt = execute_single_row_request(query);
	int rc;

	for (int i = 0; i < sqlite3_column_count(stmt); i++) {
		const char *columnName = sqlite3_column_name(stmt, i);

		if 			(strcmp(columnName, "ProgramID") == 0) {
			program.id = sqlite3_column_int(stmt, i);
		} else if 	(strcmp(columnName, "Name") == 0) {
			sprintf(program.name, (char *) sqlite3_column_text(stmt, i));
		} else if 	(strcmp(columnName, "Vision") == 0) {
			sprintf(program.vision, (char *) sqlite3_column_text(stmt, i));
		} else {
			fprintf(stderr, "!!!!!! %s not yet implemented for Program !!!!!\n", columnName);
		}
	}
	sqlite3_finalize(stmt);

	return program;
}

int db_get_number_of_tbd_primary_objectives(int mission_id) {
	char query[255];

	sprintf(query, "SELECT COUNT(*) FROM MissionObjective WHERE MissionID = %d AND Status = 0 AND ObjectiveRank = 1;", mission_id);
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return 0;

	int amt = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);

	return amt;
}

int db_get_number_of_achieved_primary_objectives(int mission_id) {
	char query[255];

	sprintf(query, "SELECT COUNT(*) FROM MissionObjective WHERE MissionID = %d AND Status = 1 AND ObjectiveRank = 1;", mission_id);
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return 0;

	int amt = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);

	return amt;
}

int db_get_number_of_failed_primary_objectives(int mission_id) {
	char query[255];

	sprintf(query, "SELECT COUNT(*) FROM MissionObjective WHERE MissionID = %d AND Status = 2 AND ObjectiveRank = 1;", mission_id);
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return 0;

	int amt = sqlite3_column_int(stmt, 0);

	sqlite3_finalize(stmt);

	return amt;
}

enum MissionSuccess db_get_mission_success(int mission_id) {
	if(db_get_number_of_tbd_primary_objectives(mission_id) > 0) return MISSION_SUCCESS_TBD;
	int num_success = db_get_number_of_achieved_primary_objectives(mission_id);
	int num_fail = db_get_number_of_failed_primary_objectives(mission_id);

	if(num_success > 0 && num_fail > 0) return MISSION_PARTIAL_SUCCESS;
	if(num_success > 0) return MISSION_SUCCESS;
	else return MISSION_FAIL;
}

int db_get_objectives_from_mission_id(struct MissionObjective_DB **p_objectives, int mission_id) {
	char query[500];
	sprintf(query, "SELECT * FROM MissionObjective WHERE MissionID = %d ORDER BY ObjectiveRank ASC;", mission_id);

	sqlite3_stmt *stmt = execute_multirow_request(query);
	int rc;

	int index = 0;
	int max_size = 10;
	struct MissionObjective_DB *objectives = (struct MissionObjective_DB *) malloc(max_size*sizeof(struct MissionObjective_DB));
	*p_objectives = objectives;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			const char *columnName = sqlite3_column_name(stmt, i);

			if 			(strcmp(columnName, "ObjectiveID") == 0) {
				objectives[index].id = sqlite3_column_int(stmt, i);
			} else if	(strcmp(columnName, "MissionID") == 0) {
				objectives[index].mission_id = sqlite3_column_int(stmt, i);
			} else if	(strcmp(columnName, "Status") == 0) {
				objectives[index].status = sqlite3_column_int(stmt, i);
			} else if	(strcmp(columnName, "ObjectiveRank") == 0) {
				objectives[index].rank = sqlite3_column_int(stmt, i)-1;
			} else if 	(strcmp(columnName, "Objective") == 0) {
				sprintf(objectives[index].objective, (char *) sqlite3_column_text(stmt, i));
			} else if 	(strcmp(columnName, "Notes") == 0) {
				const unsigned char *text = sqlite3_column_text(stmt, i);
				if (text != NULL) sprintf(objectives[index].notes, "%s", (const char *) text);
				else sprintf(objectives[index].notes, "");
			} else {
				fprintf(stderr, "!!!!!! %s not yet implemented for Mission !!!!!\n", columnName);
			}
		}
		index++;
		if(index >= max_size) {
			max_size *= 2;
			struct MissionObjective_DB *temp = (struct MissionObjective_DB *) realloc(objectives, max_size*sizeof(struct MissionObjective_DB));
			if(temp == NULL) {
				fprintf(stderr, "!!!!!! Reallocation failed for missions from database !!!!!!!!!!");
				return index;
			}
			objectives = temp;
			*p_objectives = objectives;
		}
	}

	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Error stepping through the result set");
	}

	sqlite3_finalize(stmt);

	return index;
}

int db_get_events_from_mission_id(struct MissionEvent_DB **p_events, int mission_id) {
	char query[500];
	sprintf(query, "SELECT EventID, MissionID, JULIANDAY(Time), Event FROM MissionEvent WHERE MissionID = %d ORDER BY Time ASC;", mission_id);

	sqlite3_stmt *stmt = execute_multirow_request(query);
	int rc;

	int index = 0;
	int max_size = 10;
	struct MissionEvent_DB *events = (struct MissionEvent_DB *) malloc(max_size * sizeof(struct MissionEvent_DB));
	*p_events = events;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			const char *columnName = sqlite3_column_name(stmt, i);

			if 			(strcmp(columnName, "EventID") == 0) {
				events[index].id = sqlite3_column_int(stmt, i);
			} else if	(strcmp(columnName, "MissionID") == 0) {
				events[index].mission_id = sqlite3_column_int(stmt, i);
			} else if	(strcmp(columnName, "JULIANDAY(Time)") == 0) {
				events[index].epoch = sqlite3_column_double(stmt, i);
			} else if	(strcmp(columnName, "Event") == 0) {
				sprintf(events[index].event, (char *) sqlite3_column_text(stmt, i));
			} else {
				fprintf(stderr, "!!!!!! %s not yet implemented for Mission !!!!!\n", columnName);
			}
		}
		index++;
		if(index >= max_size) {
			max_size *= 2;
			struct MissionEvent_DB *temp = (struct MissionEvent_DB *) realloc(events, max_size * sizeof(struct MissionEvent_DB));
			if(temp == NULL) {
				fprintf(stderr, "!!!!!! Reallocation failed for missions from database !!!!!!!!!!");
				return index;
			}
			events = temp;
			*p_events = events;
		}
	}

	if (rc != SQLITE_DONE) {
		fprintf(stderr, "Error stepping through the result set");
	}

	sqlite3_finalize(stmt);

	return index;
}

int db_get_last_inserted_id() {
	return db_get_id_of_last_inserted_row();
}

int is_initial_event(const char *event) {
	for(int i = 0; i < NUM_INIT_EVENTS_DESIGNATORS; i++) {
		if(strcmp(event, INIT_EVENTS_DESIGNATORS[i]) == 0) return 1;
	}
	return 0;
}

