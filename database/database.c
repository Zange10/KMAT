#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

sqlite3 *db;

int execute_query(const char *query) {
	sqlite3_stmt *stmt;

	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
	printf("%s\n", query);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return SQLITE_ERROR;
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	return SQLITE_OK;
}

int db_new_program(const char *program_name, const char *vision) {
	char query[500];
	sprintf(query, "INSERT INTO Program (Name, Vision) "
				   "VALUES ('%s', '%s');", program_name, vision);
	if(execute_query(query) != SQLITE_OK) return -1;
	return (int) sqlite3_last_insert_rowid(db);
}

int db_new_mission(const char *mission_name, int program_id) {
	char query[500];
	sprintf(query, "INSERT INTO Mission_DB (Name, ProgramID) "
				   "VALUES ('%s', %d);", mission_name, program_id);
	if(execute_query(query) != SQLITE_OK) return -1;
	return (int) sqlite3_last_insert_rowid(db);
}

int db_new_lv_family(const char *family_name) {
	char query[500];
	sprintf(query, "INSERT INTO LauncherFamily (Name) "
				   "VALUES ('%s');", family_name);
	if(execute_query(query) != SQLITE_OK) return -1;
	return (int) sqlite3_last_insert_rowid(db);
}

int db_new_lv(const char *lv_name, int family_id, double payload_diameter, double A, double c_d,
				  double leo, double sso, double gto, double tli, double tvi) {
	char query[500];
	sprintf(query, "INSERT INTO LV (Name, FamilyID, PayloadDiameter, A, c_d, LEO, SSO, GTO, TLI, TVI) "
				   "VALUES ('%s', %d, %f, %f, %f, %f, %f, %f, %f, %f);",
				   lv_name, family_id, payload_diameter, A, c_d, leo, sso, gto, tli, tvi);
	if(execute_query(query) != SQLITE_OK) return -1;
	return (int) sqlite3_last_insert_rowid(db);
}

int db_get_missions(struct Mission_DB **p_missions) {
	const char *query = "SELECT * FROM Mission_DB;";
	sqlite3_stmt *stmt;

	int rc = sqlite3_prepare(db, query, -1, &stmt, 0);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

	int index = 0;
	int max_size = 10;
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
				fprintf(stderr, "!!!!!! %s not yet implemented for Mission !!!!!\n", columnName);
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
		fprintf(stderr, "Error stepping through the result set: %s\n", sqlite3_errmsg(db));
	}

	sqlite3_finalize(stmt);

	return index;
}

struct PlaneInfo_DB db_get_plane_from_id(int id) {
	char query[48];
	sprintf(query, "SELECT * FROM Plane WHERE PlaneID = %d", id);
	sqlite3_stmt *stmt;
	struct PlaneInfo_DB fv = {-1};

	int rc = sqlite3_prepare(db, query, -1, &stmt, 0);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return fv;
	}

	rc = sqlite3_step(stmt);

	if (rc != SQLITE_ROW) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return fv;
	}

	for (int i = 0; i < sqlite3_column_count(stmt); i++) {
		const char *columnName = sqlite3_column_name(stmt, i);

		if 			(strcmp(columnName, "PlaneID") == 0) {
			fv.id = sqlite3_column_int(stmt, i);
		} else if 	(strcmp(columnName, "Name") == 0) {
			sprintf(fv.name, (char *) sqlite3_column_text(stmt, i));
		} else if 	(strcmp(columnName, "Status") == 0) {
			int status = sqlite3_column_int(stmt, i);
			fv.status = status == 1 ? ACTIVE : INACTIVE;
		} else if 	(strcmp(columnName, "RatedAltitude") == 0) {
			fv.rated_alt = sqlite3_column_int(stmt, i);
		} else if 	(strcmp(columnName, "F_vac") == 0) {
			fv.F_vac = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "F_sl") == 0) {
			fv.F_sl = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "Burnrate") == 0) {
			fv.burnrate = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "Mass_wet") == 0) {
			fv.mass_wet = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "Mass_dry") == 0) {
			fv.mass_dry = sqlite3_column_double(stmt, i);
		} else {
			fprintf(stderr, "!!!!!! %s not yet implemented for Plane !!!!!\n", columnName);
		}
	}
	sqlite3_finalize(stmt);

	return fv;
}

struct MissionProgram_DB db_get_program_from_id(int id) {
	char query[48];
	sprintf(query, "SELECT * FROM Program WHERE ProgramID = %d", id);
	sqlite3_stmt *stmt;
	struct MissionProgram_DB program = {-1};

	int rc = sqlite3_prepare(db, query, -1, &stmt, 0);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return program;
	}

	rc = sqlite3_step(stmt);

	if (rc != SQLITE_ROW) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return program;
	}

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

void init_db() {
	int rc = sqlite3_open("test.db", &db);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		return;
	}
}

void close_db() {
	sqlite3_close(db);
}

sqlite3 *get_db() {
	return db;
}
