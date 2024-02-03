#include "database.h"
#include <sqlite3.h>
#include <stdio.h>

int execute_query(sqlite3 *db, const char *query) {
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

int create_new_program(sqlite3 *db, const char *program_name, const char *vision) {
	char query[500];
	sprintf(query, "INSERT INTO Program (Name, Vision) "
				   "VALUES ('%s', '%s');", program_name, vision);
	if(execute_query(db, query) != SQLITE_OK) return -1;
	return (int) sqlite3_last_insert_rowid(db);
}

int create_new_mission(sqlite3 *db, const char *mission_name, int program_id) {
	char query[500];
	sprintf(query, "INSERT INTO Mission (Name, ProgramID) "
				   "VALUES ('%s', %d);", mission_name, program_id);
	if(execute_query(db, query) != SQLITE_OK) return -1;
	return (int) sqlite3_last_insert_rowid(db);
}

int create_new_lv_family(sqlite3 *db, const char *family_name) {
	char query[500];
	sprintf(query, "INSERT INTO LauncherFamily (Name) "
				   "VALUES ('%s');", family_name);
	if(execute_query(db, query) != SQLITE_OK) return -1;
	return (int) sqlite3_last_insert_rowid(db);
}

int create_new_lv(sqlite3 *db, const char *lv_name, int family_id, double payload_diameter, double A, double c_d,
				  double leo, double sso, double gto, double tli, double tvi) {
	char query[500];
	sprintf(query, "INSERT INTO LV (Name, FamilyID, PayloadDiameter, A, c_d, LEO, SSO, GTO, TLI, TVI) "
				   "VALUES ('%s', %d, %f, %f, %f, %f, %f, %f, %f, %f);",
				   lv_name, family_id, payload_diameter, A, c_d, leo, sso, gto, tli, tvi);
	if(execute_query(db, query) != SQLITE_OK) return -1;
	return (int) sqlite3_last_insert_rowid(db);
}

void test_db() {
	sqlite3 *db;
	int rc = sqlite3_open("test.db", &db);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		return;
	}



	sqlite3_close(db);
}