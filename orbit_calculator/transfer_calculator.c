#include "transfer_calculator.h"
#include "transfer_tools.h"
#include "csv_writer.h"
#include "tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

enum Transfer_Type final_tt = circcap;

void simple_transfer() {
    struct Body *bodies[2] = {EARTH(), JUPITER()};
    struct Date min_dep_date = {1977, 1, 1, 0, 0, 0};
    struct Date max_dep_date = {1978, 1, 1, 0, 0, 0};
    int min_duration = 400;         // [days]
    int max_duration = 1500;         // [days]
    double dep_time_steps = 24 * 60 * 60; // [seconds]
    double arr_time_steps = 24 * 60 * 60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    double jd_max_arr = jd_max_dep + max_duration;

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(jd_max_arr - jd_min_dep) / ephem_time_steps + 1;

    struct Ephem **ephems = (struct Ephem**) malloc(2*sizeof(struct Ephem*));
    for(int i = 0; i < 2; i++) {
        ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
        get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_min_dep, jd_max_arr, 1);
    }

    struct Porkchop_Properties pochopro = {
            jd_min_dep,
            jd_max_dep,
            dep_time_steps,
            arr_time_steps,
            min_duration,
            max_duration,
            ephems,
            bodies[0],
            bodies[1]
    };
    // 4 = dep_time, duration, dv1, dv2
    int data_length = 4;
    int all_data_size = (int) (data_length * (max_duration - min_duration) / (arr_time_steps / (24 * 60 * 60)) *
                               (jd_max_dep - jd_min_dep) / (dep_time_steps / (24 * 60 * 60))) + 1;
    double *porkchop = (double *) malloc(all_data_size * sizeof(double));

    create_porkchop(pochopro, circcirc, porkchop);
    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
    write_csv(data_fields, porkchop);

    int mind = 0;
    double min = 1e9;

    for(int i = 0; i < porkchop[0]/4; i++) {
        if(porkchop[i*4+4] < min) {
            mind = i;
            min = porkchop[i*4+4];
        }
    }

    double t_dep = porkchop[mind*4+1];
    double t_arr = t_dep + porkchop[mind*4+2];

    free(porkchop);

    struct OSV s0 = osv_from_ephem(ephems[0], t_dep, SUN());
    struct OSV s1 = osv_from_ephem(ephems[1], t_arr, SUN());

    double data[3];
    struct Transfer transfer = calc_transfer(circcirc, bodies[0], bodies[1], s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24*60*60), data);
    printf("Departure: ");
    print_date(convert_JD_date(t_dep),0);
    printf(", Arrival: ");
    print_date(convert_JD_date(t_arr),0);
    printf(" (%f days), Delta-v: %f m/s (%f m/s, %f m/s)\n",
           t_arr-t_dep, data[1]+data[2], data[1], data[2]);

    int num_states = 4;

    double times[] = {t_dep, t_dep, t_arr, t_arr};
    struct OSV osvs[num_states];
    osvs[0] = s0;
    osvs[1].r = transfer.r0;
    osvs[1].v = transfer.v0;
    osvs[2].r = transfer.r1;
    osvs[2].v = transfer.v1;
    osvs[3] = s1;

    double transfer_data[num_states*7+1];
    transfer_data[0] = 0;

    for(int i = 0; i < num_states; i++) {
        transfer_data[i*7+1] = times[i];
        transfer_data[i*7+2] = osvs[i].r.x;
        transfer_data[i*7+3] = osvs[i].r.y;
        transfer_data[i*7+4] = osvs[i].r.z;
        transfer_data[i*7+5] = osvs[i].v.x;
        transfer_data[i*7+6] = osvs[i].v.y;
        transfer_data[i*7+7] = osvs[i].v.z;
        transfer_data[0] += 7;
    }

    free(ephems[0]);
    free(ephems[1]);
    free(ephems);

    char pcsv;
    printf("Write transfer data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if (pcsv == 'y' || pcsv == 'Y') {
        char transfer_data_fields[] = "JD,X,Y,Z,VX,VY,VZ";
        write_csv(transfer_data_fields, transfer_data);
    }

}

void create_swing_by_transfer() {
    struct timeval start, end;
    double elapsed_time;

    struct Body *bodies[3] = {EARTH(), JUPITER(), SATURN()};
    int num_bodies = (int) (sizeof(bodies)/sizeof(struct Body*));

    struct Date min_dep_date = {1977, 3, 1, 0, 0, 0};
    struct Date max_dep_date = {1977, 11, 1, 0, 0, 0};
    int min_duration[2] = {500, 700};//, 1800, 1500};         // [days]
    int max_duration[2] = {1000, 1200};//, 2600, 2200};         // [days]
    double dep_time_steps = 24 * 60 * 60; // [seconds]
    double arr_time_steps = 24 * 60 * 60; // [seconds]

    double jd_min_dep = convert_date_JD(min_dep_date);
    double jd_max_dep = convert_date_JD(max_dep_date);

    int max_total_dur = 0;
    for(int i = 0; i < num_bodies-1; i++) max_total_dur += max_duration[i];
    double jd_max_arr = jd_max_dep + max_total_dur;

    int ephem_time_steps = 10;  // [days]
    int num_ephems = (int)(jd_max_arr - jd_min_dep) / ephem_time_steps + 1;

    struct Ephem **ephems = (struct Ephem**) malloc(num_bodies*sizeof(struct Ephem*));
    for(int i = 0; i < num_bodies; i++) {
        int ephem_available = 0;
        for(int j = 0; j < i; j++) {
            if(bodies[i] == bodies[j]) {
                ephems[i] = ephems[j];
                ephem_available = 1;
                break;
            }
        }
        if(ephem_available) continue;
        ephems[i] = (struct Ephem*) malloc(num_ephems*sizeof(struct Ephem));
        get_ephem(ephems[i], num_ephems, bodies[i]->id, ephem_time_steps, jd_min_dep, jd_max_arr, 1);
    }


    double ** porkchops = (double**) malloc((num_bodies-1) * sizeof(double*));

    for(int i = 0; i < num_bodies-1; i++) {
        double min_dep, max_dep;
        if(i == 0) {
            min_dep = jd_min_dep;
            max_dep = jd_max_dep;
        } else {
            min_dep = get_min_arr_from_porkchop(porkchops[i-1]);
            max_dep = get_max_arr_from_porkchop(porkchops[i-1]);
        }

        struct Porkchop_Properties pochopro = {
                min_dep,
                max_dep,
                dep_time_steps,
                arr_time_steps,
                min_duration[i],
                max_duration[i],
                ephems+i,
                bodies[i],
                bodies[i+1]
        };
        // 4 = dep_time, duration, dv1, dv2
        int data_length = 4;
        int all_data_size = (int) (data_length * (max_duration[i] - min_duration[i]) / (arr_time_steps / (24 * 60 * 60)) *
                                   (max_dep - min_dep) / (dep_time_steps / (24 * 60 * 60))) + 1;
        porkchops[i] = (double *) malloc(all_data_size * sizeof(double));

        gettimeofday(&start, NULL);  // Record the starting time
        if(i<num_bodies-2) create_porkchop(pochopro, circcirc, porkchops[i]);
        else               create_porkchop(pochopro, final_tt, porkchops[i]);

        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        printf("Elapsed time: %f seconds\n", elapsed_time);


        gettimeofday(&start, NULL);  // Record the starting time
        decrease_porkchop_size(i, porkchops, ephems, bodies);
        gettimeofday(&end, NULL);  // Record the ending time
        elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        printf("Elapsed time: %f seconds\n", elapsed_time);
    }

    double *jd_dates = (double*) malloc(num_bodies*sizeof(double));

    int min_total_dur = 0;
    for(int i = 0; i < num_bodies-1; i++) min_total_dur += min_duration[i];

    double *final_porkchop = (double *) malloc((int)(((max_total_dur-min_total_dur)*(jd_max_dep-jd_min_dep))*4+1)*sizeof(double));
    final_porkchop[0] = 0;

    gettimeofday(&start, NULL);  // Record the starting time
    double dep_time_info[] = {(jd_max_dep-jd_min_dep)*86400/dep_time_steps+1, dep_time_steps, jd_min_dep};
    generate_final_porkchop(porkchops, num_bodies, jd_dates, final_porkchop, dep_time_info);

    show_progress("Looking for cheapest transfer", 1, 1);
    printf("\n");

    gettimeofday(&end, NULL);  // Record the ending time
    elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Elapsed time: %f seconds\n", elapsed_time);

    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
    write_csv(data_fields, final_porkchop);

    free(final_porkchop);
    for(int i = 0; i < num_bodies-1; i++) free(porkchops[i]);
    free(porkchops);

    // 3 states per transfer (departure, arrival and final state)
    // 1 additional for initial start; 7 variables per state
    double transfer_data[((num_bodies-1)*3+1) * 7 + 1];
    transfer_data[0] = 0;

    struct OSV init_s = osv_from_ephem(ephems[0], jd_dates[0], SUN());

    transfer_data[1] = jd_dates[0];
    transfer_data[2] = init_s.r.x;
    transfer_data[3] = init_s.r.y;
    transfer_data[4] = init_s.r.z;
    transfer_data[5] = init_s.v.x;
    transfer_data[6] = init_s.v.y;
    transfer_data[7] = init_s.v.z;
    transfer_data[0] += 7;

    double dep_dv;

    for(int i = 0; i < num_bodies-1; i++) {
        struct OSV s0 = osv_from_ephem(ephems[i], jd_dates[i], SUN());
        struct OSV s1 = osv_from_ephem(ephems[i+1], jd_dates[i+1], SUN());

        double data[3];
        struct Transfer transfer;
        if(i < num_bodies-2) {
            transfer = calc_transfer(circcirc, bodies[i], bodies[i + 1], s0.r, s0.v, s1.r, s1.v,
                                     (jd_dates[i + 1] - jd_dates[i]) * (24 * 60 * 60), data);
        } else {
            transfer = calc_transfer(final_tt, bodies[i], bodies[i + 1], s0.r, s0.v, s1.r, s1.v,
                                     (jd_dates[i + 1] - jd_dates[i]) * (24 * 60 * 60), data);
        }

        if(i == 0) {
            printf("| T+    0 days (");
            print_date(convert_JD_date(jd_dates[i]),0);
            char *spacing = "                 ";
            printf(")%s- %s (%.2f m/s)\n", spacing, bodies[i]->name, data[1]);
            dep_dv = data[1];
        }
        printf("| T+% 5d days (", (int)(jd_dates[i+1]-jd_dates[0]));
        print_date(convert_JD_date(jd_dates[i+1]),0);
        printf(") - |% 5d days | - %s", (int)data[0], bodies[i+1]->name);
        if(i == num_bodies-2) printf(" (%.2f m/s)\nTotal: %.2f m/s\n", data[2], dep_dv+data[2]);
        else printf("\n");

        struct OSV osvs[3];
        osvs[0].r = transfer.r0;
        osvs[0].v = transfer.v0;
        osvs[1].r = transfer.r1;
        osvs[1].v = transfer.v1;
        osvs[2] = s1;

        for (int j = 0; j < 3; j++) {
            transfer_data[(int)transfer_data[0] + 1] = j == 0 ? jd_dates[i] : jd_dates[i+1];
            transfer_data[(int)transfer_data[0] + 2] = osvs[j].r.x;
            transfer_data[(int)transfer_data[0] + 3] = osvs[j].r.y;
            transfer_data[(int)transfer_data[0] + 4] = osvs[j].r.z;
            transfer_data[(int)transfer_data[0] + 5] = osvs[j].v.x;
            transfer_data[(int)transfer_data[0] + 6] = osvs[j].v.y;
            transfer_data[(int)transfer_data[0] + 7] = osvs[j].v.z;
            transfer_data[0] += 7;
        }
    }

    for(int i = 0; i < num_bodies; i++) {
        int ephem_double = 0;
        for(int j = 0; j < i; j++) {
            if(bodies[i] == bodies[j]) {
                ephems[i] = ephems[j];
                ephem_double = 1;
                break;
            }
        }
        if(ephem_double) continue;
        free(ephems[i]);
    }
    free(ephems);
    free(jd_dates);

    char pcsv;
    printf("Write transfer data to .csv (y/Y=yes)? ");
    scanf(" %c", &pcsv);
    if (pcsv == 'y' || pcsv == 'Y') {
        char transfer_data_fields[] = "JD,X,Y,Z,VX,VY,VZ";
        write_csv(transfer_data_fields, transfer_data);
    }
}
