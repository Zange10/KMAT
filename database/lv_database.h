#ifndef KSP_LV_DATABASE_H
#define KSP_LV_DATABASE_H

#include "database.h"


struct LauncherInfo_DB {
	int id;
	char name[30];
	int family_id;
	double leo, sso, gto, tli, tvi;
	double payload_diameter, A, c_d;
	enum VehicleStatus_DB status;
};

struct LaunchProfile_DB {
	int profiletype;
	double lp_params[5];
	char usage[128];
};

struct LaunchProfiles_DB {
	int num_profiles;
	struct LaunchProfile_DB *profile;
};

struct LV get_lv_from_database(int id);
struct LauncherInfo_DB db_get_launcherInfo_from_id(int id);
struct LaunchProfiles_DB db_get_launch_profiles_from_lv_id(int id);

#endif //KSP_LV_DATABASE_H
