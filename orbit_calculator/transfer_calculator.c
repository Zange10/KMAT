#include "transfer_calculator.h"
#include "transfer_tools.h"
#include "csv_writer.h"
#include "tool_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

enum Transfer_Type final_tt = circfb;

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
/*
    struct Ephem last_eph0 = get_last_ephem(ephems[0], t_dep);
    struct Vector r0 = {last_eph0.x, last_eph0.y, last_eph0.z};
    struct Vector v0 = {last_eph0.vx, last_eph0.vy, last_eph0.vz};
    double dt0 = (t_dep-last_eph0.date)*(24*60*60);
    struct OSV s0 = propagate_orbit(r0, v0, dt0, SUN());
    struct Ephem last_eph1 = get_last_ephem(ephems[1], t_arr);
    struct Vector r1 = {last_eph1.x, last_eph1.y, last_eph1.z};
    struct Vector v1 = {last_eph1.vx, last_eph1.vy, last_eph1.vz};
    double dt1 = (t_arr-last_eph1.date)*(24*60*60);
    struct OSV s1 = propagate_orbit(r1, v1, dt1, SUN());*/

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
    struct Body *bodies[3] = {EARTH(), JUPITER(), PLUTO()};//, URANUS(), NEPTUNE()};
    int num_bodies = (int) (sizeof(bodies)/sizeof(struct Body*));

    struct Date min_dep_date = {1980, 1, 1, 0, 0, 0};
    struct Date max_dep_date = {1985, 1, 1, 0, 0, 0};
    int min_duration[2] = {750, 1000};//, 1800, 1500};         // [days]
    int max_duration[2] = {1000, 3000};//, 2600, 2200};         // [days]
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

        if(i<num_bodies-2) create_porkchop(pochopro, circcirc, porkchops[i]);
        else               create_porkchop(pochopro, final_tt, porkchops[i]);

        if(i == 0) {
            double min_dep_dv = get_min_from_porkchop(porkchops[0], 3);
            double *temp = (double*) malloc(((int)porkchops[0][0]+1)*sizeof(double));
            temp[0] = 0;
            for(int j = 0; j < (int)(porkchops[0][0]/4); j++) {
                show_progress("Decreasing Porkchop size", (double)j, (porkchops[0][0]/4));
                if(porkchops[0][j*4 + 3] < 2*min_dep_dv) {
                    for(int k = 1; k <= 4; k++) {
                        temp[(int)temp[0] + k] = porkchops[0][j*4 + k];
                    }
                    temp[0] += 4;
                }
            }
            show_progress("Decreasing Porkchop size", 1, 1);
            printf("\nTrajectories remaining: %d\n", (int)temp[0]/4);
            free(porkchops[0]);
            porkchops[0] = realloc(temp, (int)(temp[0]+1)*sizeof(double));
        } else {
            double *temp = (double*) malloc(((int)porkchops[i][0]+1)*sizeof(double));
            temp[0] = 0;
            for(int j = 0; j < (int)(porkchops[i][0]/4); j++) {
                show_progress("Finding fly-bys", (double)j, (porkchops[i][0]/4));
                for(int k = 0; k < (int)(porkchops[i-1][0]/4); k++) {
                    if(porkchops[i-1][k * 4 + 1] + porkchops[i-1][k * 4 + 2] != porkchops[i][j * 4 + 1]) continue;
                    double arr_v = porkchops[i-1][k * 4 + 4];
                    double dep_v = porkchops[i][j * 4 + 3];
                    if(fabs(arr_v - dep_v) < 10) {
                        double data[3]; // not used (just as parameter for calc_transfer)
                        double t_dep = porkchops[i-1][k * 4 + 1];
                        double t_arr = porkchops[i][j * 4 + 1];
                        struct OSV s0 = osv_from_ephem(ephems[i-1], t_dep, SUN());
                        struct OSV s1 = osv_from_ephem(ephems[i], t_arr, SUN());
                        struct Transfer transfer1 = calc_transfer(circcirc, bodies[i], bodies[i+1], s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24 * 60 * 60), data);

                        t_dep = porkchops[i][j * 4 + 1];
                        t_arr = porkchops[i][j * 4 + 1]+porkchops[i][j * 4 + 2];
                        s0 = s1;
                        s1 = osv_from_ephem(ephems[i+1], t_arr, SUN());
                        struct Transfer transfer2 = calc_transfer(circcirc, bodies[i], bodies[i+1], s0.r, s0.v, s1.r, s1.v, (t_arr-t_dep) * (24 * 60 * 60), data);

                        struct Vector temp1 = add_vectors(transfer1.v1, scalar_multiply(s0.v,-1));
                        struct Vector temp2 = add_vectors(transfer2.v0, scalar_multiply(s0.v,-1));
                        double beta = (M_PI-angle_vec_vec(temp1, temp2))/2;
                        double rp = (1/cos(beta)-1)*(bodies[i]->mu/(pow(vector_mag(temp1), 2)));
                        if(rp > bodies[i]->radius+bodies[i]->atmo_alt) {
                            for (int l = 1; l <= 4; l++) {
                                temp[(int) temp[0] + l] = porkchops[i][j * 4 + l];
                            }
                            temp[0] += 4;
                            break;  // only need to store this transfer once
                        }
                    }
                }
            }
            show_progress("Finding fly-bys", 1, 1);
            if(temp[0]==0) {
                printf("\nNo trajectories found\n");
                return;
            }
            printf("\nTrajectories remaining: %d\n", (int)temp[0]/4);
            free(porkchops[i]);
            porkchops[i] = realloc(temp, (int)(temp[0]+1)*sizeof(double));
        }
    }

    double *jd_dates = (double*) malloc(num_bodies*sizeof(double));
    double min = 1e9;

    int min_total_dur = 0;
    for(int i = 0; i < num_bodies-1; i++) min_total_dur += min_duration[i];

    double *final_porkchop = (double *) malloc((int)(((max_total_dur-min_total_dur)*(jd_max_dep-jd_min_dep))*4+1)*sizeof(double));
    final_porkchop[0] = 0;

    for(int i = 0; i < (int)(porkchops[0][0]/4); i++) {
        show_progress("Looking for cheapest transfer", (double)i, (porkchops[0][0]/4));
        double *p_dep = porkchops[0]+i*4;
        double dv_dep = p_dep[3];
        double *current_dates = (double*) malloc(num_bodies*sizeof(double));
        current_dates[0] = p_dep[1];
        get_cheapest_transfer_dates(porkchops, p_dep, dv_dep, 1, num_bodies-1, current_dates, jd_dates, &min, final_porkchop);
        free(current_dates);
    }
    show_progress("Looking for cheapest transfer", 1, 1);
    printf("\n");

    char data_fields[] = "dep_date,duration,dv_dep,dv_arr";
    write_csv(data_fields, final_porkchop);

    free(final_porkchop);
    for(int i = 0; i < num_bodies-1; i++) free(porkchops[i]);
    free(porkchops);

    for(int i = 0; i < num_bodies; i++) print_date(convert_JD_date(jd_dates[i]),1);

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
        printf("Departure: ");
        print_date(convert_JD_date(jd_dates[i]), 0);
        printf(", Arrival: ");
        print_date(convert_JD_date(jd_dates[i+1]), 0);
        printf(" (%f days), Delta-v: %f m/s (%f m/s, %f m/s)\n",
               jd_dates[i+1] - jd_dates[i], data[1] + data[2], data[1], data[2]);


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

    for(int i = 0; i < num_bodies; i++) free(ephems[i]);
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
