#include "lv_database.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "launch_calculator/lv_profile.h"


void db_get_stages_from_lv_id(struct LV *lv, int id);
int db_get_all_launcher_id(int **lv_id);


struct LV get_lv_from_database(int id) {
	struct LV lv;
	struct LauncherInfo_DB lv_db = db_get_launcherInfo_from_id(id);
	sprintf(lv.name, "%s", lv_db.name);
	lv.c_d = lv_db.c_d;
	lv.A = lv_db.A;
	
	db_get_stages_from_lv_id(&lv, id);
	
	return lv;
}

int get_all_launch_vehicles_from_database(struct LV **all_lvs, int **lv_id) {
	int num_lv = db_get_all_launcher_id(lv_id);
	
	*all_lvs = (struct LV*) malloc(sizeof(struct LV) * num_lv);
	for(int i = 0; i < num_lv; i++) {
		(*all_lvs)[i] = get_lv_from_database((*lv_id)[i]);
	}
	
	return num_lv;
}


int db_get_all_launcher_id(int **lv_id) {
	char query[48];
	sprintf(query, "SELECT COUNT(*) FROM LV");
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return 0;
	
	int num_lv = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	
	sprintf(query, "SELECT LauncherID FROM LV ORDER BY Name ASC");
	stmt = execute_multirow_request(query);
	if(stmt == NULL) return 0;
	
	int rc;
	*lv_id = (int*) malloc(sizeof(int)*num_lv);
	
	int lv_index = 0;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		(*lv_id)[lv_index] = sqlite3_column_int(stmt, 0);
		lv_index++;
	}
	
	sqlite3_finalize(stmt);
	return num_lv;
}

struct LauncherInfo_DB db_get_launcherInfo_from_id(int id) {
	char query[48];
	
	sprintf(query, "SELECT * FROM LV WHERE LauncherID = %d", id);
	struct LauncherInfo_DB lv = {-1};
	
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return lv;
	
	for (int i = 0; i < sqlite3_column_count(stmt); i++) {
		const char *columnName = sqlite3_column_name(stmt, i);
		
		if 			(strcmp(columnName, "LauncherID") == 0) {
			lv.id = sqlite3_column_int(stmt, i);
		} else if 	(strcmp(columnName, "FamilyID") == 0) {
			lv.family_id = sqlite3_column_int(stmt, i);
		} else if 	(strcmp(columnName, "Name") == 0) {
			sprintf(lv.name, (char *) sqlite3_column_text(stmt, i));
		} else if 	(strcmp(columnName, "Status") == 0) {
			int status = sqlite3_column_int(stmt, i);
			lv.status = status == 1 ? ACTIVE : INACTIVE;
		} else if 	(strcmp(columnName, "LEO") == 0) {
			lv.leo = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "SSO") == 0) {
			lv.sso = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "GTO") == 0) {
			lv.gto = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "TLI") == 0) {
			lv.tli = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "TVI") == 0) {
			lv.tvi = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "PayloadDiameter") == 0) {
			lv.payload_diameter = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "A") == 0) {
			lv.A = sqlite3_column_double(stmt, i);
		} else if 	(strcmp(columnName, "c_d") == 0) {
			lv.c_d = sqlite3_column_double(stmt, i);
		} else {
			//fprintf(stderr, "!!!!!! %s not yet implemented for LV !!!!!\n", columnName);
		}
	}
	sqlite3_finalize(stmt);
	
	return lv;
}

int db_get_number_of_stages_from_lv_id(int id) {
	char query[255];
	
	sprintf(query, "SELECT COUNT(*) FROM LV_Stage WHERE LauncherID = %d", id);
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return 0;
	
	int num_stages = sqlite3_column_int(stmt, 0);
	
	sqlite3_finalize(stmt);
	
	return num_stages;
}

void db_get_stages_from_lv_id(struct LV *lv, int id) {
	lv->stage_n = db_get_number_of_stages_from_lv_id(id);
	lv->stages = (struct Stage*) malloc(sizeof(struct Stage) * lv->stage_n);
	
	char query[255];

	sprintf(query, "SELECT * FROM Stage s INNER JOIN LV_Stage ls ON s.StageID = ls.StageID WHERE ls.LauncherID = %d ORDER BY ls.Stagenumber ASC", id);
	sqlite3_stmt *stmt = execute_multirow_request(query);
	if(stmt == NULL) return;
	int rc;

	
	int stage_index = 0;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			const char *columnName = sqlite3_column_name(stmt, i);
			
			if 			(strcmp(columnName, "F_vac") == 0) {
				lv->stages[stage_index].F_vac = sqlite3_column_double(stmt, i) * 1000;
			} else if 	(strcmp(columnName, "F_sl") == 0) {
				lv->stages[stage_index].F_sl = sqlite3_column_double(stmt, i) * 1000;
			} else if 	(strcmp(columnName, "Mass_wet") == 0) {
				lv->stages[stage_index].m0 = sqlite3_column_double(stmt, i) * 1000;
			} else if 	(strcmp(columnName, "Mass_dry") == 0) {
				lv->stages[stage_index].me = sqlite3_column_double(stmt, i) * 1000;
			} else if 	(strcmp(columnName, "Burnrate") == 0) {
				lv->stages[stage_index].burn_rate = sqlite3_column_double(stmt, i);
			} else if 	(strcmp(columnName, "Stagenumber") == 0) {
				lv->stages[stage_index].stage_id = sqlite3_column_int(stmt, i);
			}
		}
		stage_index++;
	}
	
	sqlite3_finalize(stmt);
}


int db_get_number_of_laumch_profiles_from_lv_id(int id) {
	char query[255];
	
	sprintf(query, "SELECT COUNT(*) FROM LV_LP WHERE LauncherID = %d", id);
	sqlite3_stmt *stmt = execute_single_row_request(query);
	if(stmt == NULL) return 0;
	
	int num_profiles = sqlite3_column_int(stmt, 0);
	
	sqlite3_finalize(stmt);
	
	return num_profiles;
}

struct LaunchProfiles_DB db_get_launch_profiles_from_lv_id(int id) {
	struct LaunchProfiles_DB profiles = {
			db_get_number_of_laumch_profiles_from_lv_id(id),
			NULL
	};
	
	if(profiles.num_profiles == 0) return profiles;
	
	char query[255];
	
	sprintf(query, "SELECT * FROM LaunchProfile lp INNER JOIN LV_LP lvlp ON lp.ProfileID = lvlp.ProfileID WHERE LauncherID = %d ORDER BY lp.ProfileType ASC", id);
	sqlite3_stmt *stmt = execute_multirow_request(query);
	if(stmt == NULL) return profiles;
	int rc;
	
	profiles.profile = (struct LaunchProfile_DB*) malloc(sizeof(struct LaunchProfile_DB) * profiles.num_profiles);
	
	int profile_index = 0;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		// Process each row of the result set
		for (int i = 0; i < sqlite3_column_count(stmt); i++) {
			const char *columnName = sqlite3_column_name(stmt, i);
			
			if 			(strcmp(columnName, "Usage") == 0) {
				sprintf(profiles.profile[profile_index].usage, (char *) sqlite3_column_text(stmt, i));
			} else if 	(strcmp(columnName, "ProfileType") == 0) {
				profiles.profile[profile_index].profiletype = sqlite3_column_int(stmt, i);
			} else if 	(strcmp(columnName, "a") == 0) {
				profiles.profile[profile_index].lp_params[0] = sqlite3_column_double(stmt, i);
			} else if 	(strcmp(columnName, "b") == 0) {
				profiles.profile[profile_index].lp_params[1] = sqlite3_column_double(stmt, i);
			} else if 	(strcmp(columnName, "c") == 0) {
				profiles.profile[profile_index].lp_params[2] = sqlite3_column_double(stmt, i);
			} else if 	(strcmp(columnName, "d") == 0) {
				profiles.profile[profile_index].lp_params[3] = sqlite3_column_double(stmt, i);
			} else if 	(strcmp(columnName, "e") == 0) {
				profiles.profile[profile_index].lp_params[4] = sqlite3_column_double(stmt, i);
			}
		}
		profile_index++;
	}
	
	sqlite3_finalize(stmt);
	
	return profiles;
}


