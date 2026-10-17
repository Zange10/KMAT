#include <stdio.h>
#include "tool_funcs.h"

#include <stdlib.h>


TimingMeasurements init_timing_measurements() {
    TimingMeasurements tm;
    tm.first = NULL;
    gettimeofday(&tm.start, NULL);
    gettimeofday(&tm.end, NULL);
    return tm;
}

void start_time_measurement(TimingMeasurements *tm) {
    gettimeofday(&tm->start, NULL);
}

double get_total_timing_time(TimingMeasurements tm) {
    TimingMeasurement *ptr = tm.first;
    double total_time = 0;
    while(ptr) {
        total_time += ptr->elapsed_time;
        ptr = ptr->next;
    }
    return total_time;
}

void print_timing_measurements(TimingMeasurements tm) {
    double total_time = get_total_timing_time(tm);
    TimingMeasurement *ptr = tm.first;
    while(ptr) {
        printf("|%50s:%10.3fms  (%.2f %%)\n", ptr->name, ptr->elapsed_time*1000, ptr->elapsed_time/total_time*100);
        ptr = ptr->next;
    }
    print_separator(100);
    printf("|%50s:  %.3fms\n", "TOTAL TIME", total_time*1000);
    print_separator(100);
}

void end_time_measurement(TimingMeasurements *tm, char *name) {
    gettimeofday(&tm->end, NULL);
    TimingMeasurement *ptr = tm->first;
    if(ptr == NULL) {
        tm->first = malloc(sizeof(TimingMeasurement));
        ptr = tm->first;
    } else {
        while(ptr->next) ptr = ptr->next;
        ptr->next = malloc(sizeof(TimingMeasurement));
        ptr = ptr->next;
    }

    ptr->next = NULL;
    ptr->elapsed_time = (tm->end.tv_sec - tm->start.tv_sec) + (tm->end.tv_usec - tm->start.tv_usec) / 1000000.0;
    sprintf(ptr->name, "%s", name);
}

TimingMeasurement *get_last_timing_measurement(TimingMeasurements tm) {
    TimingMeasurement *ptr = tm.first;
    while(ptr->next) {
        ptr = ptr->next;
    }
    return ptr;
}

void free_timing_measurements(TimingMeasurements *tm) {
    if(!tm) return;
    if(tm->first) {
        TimingMeasurement *ptr = tm->first;
        while(ptr) {
            TimingMeasurement *next = ptr->next;
            free(ptr);
            ptr = next;
        }
    }
}




int user_selection(char *title, char *options, char *question) {
    printf("\n\n%s\n", title);
    print_separator(49);
    int i = 1;  // 1, because see while loop
    int j = 0;

    while(options[i-1] != 0) {  // -1, because i is the first character of the option (character in front is relevant)
        if(options[0] == ' ') options = &options[i+1];      // skip first character if space

        while(options[i] != 0 && options[i] != ';') i++;

        printf("| - % 2d: %.*s", j, i, options);
        for(int k = 0; k < (47-(i+8))/8; k++) printf("\t");     // for a max line length of 32 (next line for initial \t)
        printf("\t|\n");

        options = &options[i+1];
        i = 0;
        j++;
    }

    print_separator(49);
    printf("\n%s", question);

    int sel;
    scanf(" %d", &sel);
    if(sel < 0 || sel >= j) sel = 0;    // if sel is not one of the options, set to 0
    printf("\n");
    print_separator(49);
    print_separator(49);
    printf("\n");
    return sel;
}


void print_separator(int x) {
    for(int i = 0; i < x; i++) printf("_");
    printf("\n\n");
}


int strcicmp(char const *a, char const *b)
{
    for (;; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d != 0 || !*a)
            return d;
    }
}


void show_progress(char *text, double progress, double total) {
    double percentage = (progress / total) * 100.0;
    printf("\r%s: %.2f%%", text, percentage);
    fflush(stdout);
}
