#include "ephem.h"
#include "datetime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#ifdef _WIN32
#include <windows.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")  // Link against urlmon.dll
#endif



struct Date ephem_min_date = {.y = 1950, .m = 1, .d = 1, .date_type = DATE_ISO};
struct Date ephem_max_date = {.y = 2100, .m = 1, .d = 1, .date_type = DATE_ISO};
char *ephem_directory = "./Ephemerides";


void get_ephem_data_filepath(int id, char *filepath) {
	sprintf(filepath, "%s/%d.ephem", ephem_directory, id);
}

int is_ephem_available(int body_code) {
	char filepath[50];
	get_ephem_data_filepath(body_code, filepath);
	FILE *file = fopen(filepath, "r");  // Try to open file in read mode
	if (file) {
		fclose(file);  // Close file if it was opened
		return 1;      // File exists
	}
	return 0;          // File does not exist
}


void print_ephem(struct Ephem ephem) {
    printf("Date: %f  (", ephem.date);
    print_date(convert_JD_date(ephem.date, DATE_ISO), 0);
    printf(")\nx: %g m,   y: %g m,   z: %g m\n"
           "vx: %g m/s,   vy: %g m/s,   vz: %g m/s\n\n",
           ephem.x, ephem.y, ephem.z, ephem.vx, ephem.vy, ephem.vz);
}

void create_directory_if_not_exists(const char *path) {
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		GError *error = NULL;
		if (g_mkdir_with_parents(path, 0755) == -1) {
			g_warning("Failed to create directory: %s", error->message);
			g_error_free(error);
		}
	}
}

void download_file(const char *url, const char *filepath) {
#ifdef _WIN32
	HRESULT hr = URLDownloadToFile(NULL, url, filepath, 0, NULL);
    if (hr != S_OK) {
        fprintf(stderr, "Error downloading file: %lx\n", hr);
    }
#else
	char wget_command[512];
	snprintf(wget_command, sizeof(wget_command), "wget \"%s\" -O %s", url, filepath);
	int ret_code = system(wget_command);
	if (ret_code != 0) {
		fprintf(stderr, "Error executing wget: %d\n", ret_code);
	}
#endif
}

void get_body_ephems(struct Body *body, struct Body *central_body) {
	char filepath[50];
	get_ephem_data_filepath(body->id, filepath);

    if(!is_ephem_available(body->id)) {
		create_directory_if_not_exists(ephem_directory);

        struct Date d0 = ephem_min_date;
        struct Date d1 = ephem_max_date;
        char d0_s[32];
        char d1_s[32];
        date_to_string(d0, d0_s, 1);
        date_to_string(d1, d1_s, 1);
        // Construct the URL with your API key and parameters

        char url[256];
        sprintf(url, "https://ssd.jpl.nasa.gov/api/horizons.api?"
                     "format=text&"
                     "COMMAND='%d'&"
                     "OBJ_DATA='NO'&"
                     "MAKE_EPHEM='YES'&"
                     "EPHEM_TYPE='VECTORS'&"
                     "CENTER='500@%d'&"
                     "START_TIME='%s'&"
                     "STOP_TIME='%s'&"
                     "STEP_SIZE='1 mo'&"
                     "VEC_TABLE='2'", body->id, central_body->id, d0_s, d1_s);

		download_file(url, filepath);
    }

	FILE *file;
	char line[256];  // Assuming lines are no longer than 255 characters

	file = fopen(filepath, "r");

	if(file == NULL) {
		perror("Unable to open file");
		return;
	}

	// Read lines from the file until the end is reached
	while(fgets(line, sizeof(line), file) != NULL) {
		line[strcspn(line, "\n")] = '\0';
		if(strcmp(line, "$$SOE") == 0) {
			break; // Exit the loop when "$$SOE" is encountered
		}
	}

	int num_ephems = 12*(ephem_max_date.y - ephem_min_date.y);

	if(body->ephem != NULL) free(body->ephem);
	body->ephem = calloc(num_ephems+1, sizeof(struct Ephem));

	for(int i = 0; i < num_ephems; i++) {
		fgets(line, sizeof(line), file);
		line[strcspn(line, "\n")] = '\0';
		if(strcmp(line, "$$EOE") == 0) break; // Exit the loop when "$$SOE" is encountered
		char *endptr;
		double date = strtod(line, &endptr);

		fgets(line, sizeof(line), file);
		double x, y, z;
		sscanf(line, " X =%lf Y =%lf Z =%lf", &x, &y, &z);
		fgets(line, sizeof(line), file);
		double vx, vy, vz;
		sscanf(line, " VX=%lf VY=%lf VZ=%lf", &vx, &vy, &vz);

		body->ephem[i].date = date;
		body->ephem[i].x = x*1e3;
		body->ephem[i].y = y*1e3;
		body->ephem[i].z = z*1e3;
		body->ephem[i].vx = vx*1e3;
		body->ephem[i].vy = vy*1e3;
		body->ephem[i].vz = vz*1e3;
	}
	fclose(file);
}

struct Ephem get_closest_ephem(struct Ephem *ephem, double date) {
	int i = 0;
	while (ephem[i].date > 0) {
		if(date < ephem[i].date) {
			if(i == 0) return ephem[0];
			if(fabs(ephem[i-1].date - date) < fabs(ephem[i].date-date)) return ephem[i-1];
			else return ephem[i];
		}
		i++;
	}
	return ephem[i-1];
}
