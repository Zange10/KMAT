#include "ephem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void print_ephem(struct Ephem ephem) {
    printf("Date: %f  (", ephem.date);
    print_date(convert_JD_date(ephem.date),0);
    printf(")\nx: %g m,   y: %g m,   z: %g m\n"
           "vx: %g m/s,   vy: %g m/s,   vz: %g m/s\n\n",
           ephem.x, ephem.y, ephem.z, ephem.vx, ephem.vy, ephem.vz);
}

void print_date(struct Date date, int line_break) {
    printf("%4d-%02d-%02d %02d:%02d:%06.3f", date.y, date.m, date.d, date.h, date.min, date.s);
    if(line_break) printf("\n");
}

void date_to_string(struct Date date, char *s, int clocktime) {
	if(clocktime) sprintf(s,"%4d-%02d-%02d %02d:%02d:%02.0f", date.y, date.m, date.d, date.h, date.min, date.s);
	else sprintf(s,"%4d-%02d-%02d", date.y, date.m, date.d);
}

struct Date date_from_string(char *s) {
	int year, month, day;	// "yyyy-mm-dd" (ISO 8601)
	char *ptr;

	// Extract year
	year = (int) strtol(s, &ptr, 10);
	ptr++; // Move past the '-'

	// Extract month
	month = (int) strtol(ptr, &ptr, 10);
	ptr++; // Move past the '-'

	// Extract day
	day = (int) strtol(ptr, &ptr, 10);
	struct Date date = {year, month, day};
	return date;
}

struct Date convert_JD_date(double JD) {
    struct Date date = {0,1,1,0,0,0};
	
    JD -= 2451544.5;
	date.y = 2000;
	if(JD < 0) {
		date.y--;
		while(JD < -365 - (date.y % 4 == 0)) {
			JD += 365 + (date.y % 4 == 0);	// boolean to add one (for leap year)
			date.y--;
		}
		JD += 365 + (date.y % 4 == 0);	// boolean to add one (for leap year)
	} else {
		while(JD > 365 + (date.y % 4 == 0)) {
			JD -= 365 + (date.y % 4 == 0);	// boolean to add one (for leap year)
			date.y++;
		}
	}
	
    for(int i = 1; i < 12; i++) {
        int month_days;
        if(i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10) month_days = 31;
        else if(i == 4 || i == 6 || i == 9 || i == 11) month_days = 30;
        else {
            if(date.y%4 == 0) month_days = 29;
            else month_days = 28;
        }
        JD -= month_days;
        if(JD < 0) {
            JD += month_days;
            break;
        } else {
            date.m++;
        }
    }

    date.d += (int)JD;
    JD -= (int)JD;

    // years before a leap year before 2000 would go to 32 Dec
    if(date.m == 12 && date.d == 32) {
        date.m = 1;
        date.d = 1;
        date.y++;
    }

    date.h = (int) (JD * 24.0);
    JD -= (double)date.h/24;
    date.min = floor(JD * 24 * 60);
    JD -= (double)date.min/(24*60);
    date.s = JD*86400;
	if(date.s > 59.999) {
		date.s = 0;
		date.min++;
		if(date.min > 59.999) {
			date.min = 0;
			date.h++;
		}
	}


    return date;
}

double convert_date_JD(struct Date date) {
    double J = 2451544.5;     // 2000-01-01 00:00
    int diff_year = date.y-2000;
    int year_part = diff_year * 365.25;
    if(date.y < 2000 || date.y%4 == 0) J -= 1; // leap years do leap day themselves after 2000

    int month_part = 0;
    for(int i = 1; i < 12; i++) {
        if(i==date.m) break;
        if(i == 1 || i == 3 || i == 5 || i == 7 || i == 8 || i == 10) month_part += 31;
        else if(i == 4 || i == 6 || i == 9 || i == 11) month_part += 30;
        else {
            if(date.y%4 == 0) month_part += 29;
            else month_part += 28;
        }
    }

    J += month_part+year_part+date.d;
    J += (double)date.h/24 + (double)date.min/(24*60) + date.s/(24*60*60);
    return J;
}

double jd_change_date(double jd, int delta_years, int delta_months, double delta_days) {
	jd += delta_days;
	struct Date date = convert_JD_date(jd);
	date.m += delta_months;
	while(date.m > 12) {
		date.m -= 12;
		date.y++;
	}
	while(date.m < 1) {
		date.m += 12;
		date.y--;
	}
	date.y += delta_years;
	jd = convert_date_JD(date);
	return jd;
}

struct Date get_date_difference_from_epochs(double jd0, double jd1) {
	double epoch_diff = jd1 - jd0;
	struct Date date = {0, 0};
	// floating-point inprecision when converting to seconds 1 -> 0.999997
	if(fmod(epoch_diff*24*60*60, 1) > 0.9) epoch_diff += 1.0/(24*60*60*10);
	date.d = (int) epoch_diff;
	date.h = (int) (epoch_diff*24) % 24;
	date.min = (int) (epoch_diff*24*60) % 60;
	date.s = (int) (epoch_diff*24*60*60) % 60;
	return date;
}

void get_ephem(struct Ephem *ephem, int size_ephem, int body_code, int time_steps, double jd0, double jd1, int download) {
    if(download) {
        struct Date d0 = convert_JD_date(jd0);
        struct Date d1 = convert_JD_date(jd1);
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
