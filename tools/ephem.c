#include "ephem.h"
#include "datetime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void print_ephem(struct Ephem ephem) {
    printf("Date: %f  (", ephem.date);
    print_date(convert_JD_date(ephem.date, DATE_ISO), 0);
    printf(")\nx: %g m,   y: %g m,   z: %g m\n"
           "vx: %g m/s,   vy: %g m/s,   vz: %g m/s\n\n",
           ephem.x, ephem.y, ephem.z, ephem.vx, ephem.vy, ephem.vz);
}

void get_ephem(struct Ephem *ephem, int size_ephem, int body_code, int time_steps, double jd0, double jd1, int download) {
    if(download) {
        struct Date d0 = convert_JD_date(jd0, DATE_ISO);
        struct Date d1 = convert_JD_date(jd1, DATE_ISO);
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
                     "CENTER='500@10'&"
                     "START_TIME='%s'&"
                     "STOP_TIME='%s'&"
                     "STEP_SIZE='%dd'&"
                     "VEC_TABLE='2'", body_code, d0_s, d1_s, time_steps);

        // Construct the wget command
        char wget_command[512];
        snprintf(wget_command, sizeof(wget_command), "wget \"%s\" -O output.json", url);

        // Execute the wget command
        int ret_code = system(wget_command);

        // Check for errors
        if (ret_code != 0) {
            fprintf(stderr, "Error executing wget: %d\n", ret_code);
            return;
        }
    }
    // Now you can parse the downloaded JSON file (output.json) in your C program.

    FILE *file;
    char line[256];  // Assuming lines are no longer than 255 characters

    if(download) file = fopen("output.json", "r");
    else{
        if(body_code == 3) file = fopen("earth.json", "r");
        else file = fopen("venus.json", "r");
    }

    if (file == NULL) {
        perror("Unable to open file");
        return;
    }

    // Read lines from the file until the end is reached
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "$$SOE") == 0) {
            break; // Exit the loop when "$$SOE" is encountered
        }
    }

    for(int i = 0; i < size_ephem-1; i++){
        fgets(line, sizeof(line), file);
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "$$EOE") == 0) {
            size_ephem = i+1;   // see below for signifying end of array
            break; // Exit the loop when "$$SOE" is encountered
        }
        char *endptr;
        double date = strtod(line, &endptr);

        fgets(line, sizeof(line), file);
        double x,y,z;
        sscanf(line, " X =%lf Y =%lf Z =%lf", &x, &y, &z);
        fgets(line, sizeof(line), file);
        double vx,vy,vz;
        sscanf(line, " VX=%lf VY=%lf VZ=%lf", &vx, &vy, &vz);

        ephem[i].date = date;
        ephem[i].x = x*1e3;
        ephem[i].y = y*1e3;
        ephem[i].z = z*1e3;
        ephem[i].vx = vx*1e3;
        ephem[i].vy = vy*1e3;
        ephem[i].vz = vz*1e3;
    }

    ephem[size_ephem-1].date = -1;  // to know where array ends
    // Close the file when done
    fclose(file);
}

void get_body_ephem(struct Ephem *ephem, int body_code) {
	for(int a = 0; a < 10; a++) {
		int year = 1950 + a*10;
		char file_path[30];
		sprintf(file_path, "Ephems/%d/%d.ephem", body_code, year);
		
		FILE *file;
		char line[256];  // Assuming lines are no longer than 255 characters
		
		file = fopen(file_path, "r");
		
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
		
		for(int i = 0; i < 12*10; i++) {
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
			
			ephem[a*120 + i].date = date;
			ephem[a*120 + i].x = x*1e3;
			ephem[a*120 + i].y = y*1e3;
			ephem[a*120 + i].z = z*1e3;
			ephem[a*120 + i].vx = vx*1e3;
			ephem[a*120 + i].vy = vy*1e3;
			ephem[a*120 + i].vz = vz*1e3;
		}
		fclose(file);
	}
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
