#include <stdio.h>
#include <time.h>

#include "csv_writer.h"

void write_csv(char fields[], double data[]) {

    time_t currtime;
    time(&currtime);
    struct tm now = *gmtime(&currtime);
    char filename[19];  // 14 for date + 4 for .csv + 1 for sprintf terminator
    sprintf(filename, "%04d%02d%02d%02d%02d%02d.csv", now.tm_year+1900, now.tm_mon, now.tm_mday,now.tm_hour, now.tm_min, now.tm_sec);

    // -------------------

    int n_fields = amt_of_fields(fields);

    FILE *file;
    file = fopen(filename,"w");

    fprintf(file,"%s\n", fields);

    for(int i = 1; i < data[0]; i+=n_fields) {   // data[0] amount of data points in data
        for(int j = 0; j < n_fields-1; j++) fprintf(file, "%G,",data[i+j]);
        fprintf(file, "%G\n", data[i+n_fields-1]);
    }

    fclose(file);
}

int amt_of_fields(char *fields) {
    int c = 1;
    int i = 0;
    do {
        if(fields[i] == ',') c++;
        i++;
    } while(fields[i] != 0);    // 0 = NULL Terminator for strings
    return c;
}