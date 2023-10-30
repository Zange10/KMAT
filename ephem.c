#include "ephem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_ephem(struct Ephem ephem) {
    printf("\nDate: %f\nx: %g m,   y: %g m,   z: %g m\n"
           "vx: %g m/s,   vy: %g m/s,   vz: %g m/s\n\n",
           ephem.date, ephem.x, ephem.y, ephem.z, ephem.vx, ephem.vy, ephem.vz);
}

void print_date(struct Date date) {
    printf("%4d-%02d-%02d %02d:%02d\n", date.y, date.m, date.d, date.h, date.min);
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
    printf("%d, %d\n", month_part, year_part);
    J += month_part+year_part+date.d;
    J += date.h/24 + date.min/60;
    return J;
}

void get_ephem() {
    // Construct the URL with your API key and parameters
    const char *url = "https://ssd.jpl.nasa.gov/api/horizons.api?"
                      "format=text&"
                      "COMMAND='2'&"
                      "OBJ_DATA='NO'&"
                      "MAKE_EPHEM='YES'&"
                      "EPHEM_TYPE='VECTORS'&"
                      "CENTER='500@0'&"
                      "START_TIME='2000-01-01'&"
                      "STOP_TIME='2005-01-01'&"
                      "STEP_SIZE='210d'&"
                      "VEC_TABLE='2'&"
                      "QUANTITIES='1,9,20,23,24,29'";

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
    // Now you can parse the downloaded JSON file (output.json) in your C program.

    FILE *file;
    char line[256];  // Assuming lines are no longer than 255 characters

    // Open the file for reading
    file = fopen("output.json", "r");

    if (file == NULL) {
        perror("Unable to open file");
        return;
    }

    struct Ephem ephem[400];

    // Read lines from the file until the end is reached
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "$$SOE") == 0) {
            break; // Exit the loop when "$$SOE" is encountered
        }
    }

    for(int i = 0; i < sizeof(ephem) / sizeof(struct Ephem); i++){
        fgets(line, sizeof(line), file);
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "$$EOE") == 0) {
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


    struct Date date = {2000, 1, 1, 0, 0};
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[0]);
    date.y = 2000; date.m = 7, date.d = 29;
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[1]);
    date.y = 2001; date.m = 2, date.d = 24;
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[2]);
    date.y = 2001; date.m = 9, date.d = 22;
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[3]);
    date.y = 2002; date.m = 4, date.d = 20;
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[4]);
    date.y = 2002; date.m = 11, date.d = 16;
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[5]);
    date.y = 2003; date.m = 6, date.d = 14;
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[6]);
    date.y = 2004; date.m = 1, date.d = 10;
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[7]);
    date.y = 2004; date.m = 8, date.d = 7;
    print_date(date);
    printf("Date: %f", convert_date_JD(date));
    print_ephem(ephem[8]);

    // Close the file when done
    fclose(file);

    return;
}
