#include <stdio.h>

#include "lv_profile.h"


struct Stage init_stage(double F_sl, double F_vac, double m0, double me, double br) {
    struct Stage new_stage;
    new_stage.F_vac = F_vac*1000;   // kN to N
    new_stage.F_sl = F_sl*1000;     // kN to N
    new_stage.m0 = m0*1000;         // t to kg
    new_stage.me = me*1000;         // t to kg
    new_stage.burn_rate = br;
    return new_stage;
}

struct LV init_LV(char * name, int amt_of_stages, struct Stage *stages, int payload_mass) {
    struct LV new_lv;
    new_lv.name = name;
    new_lv.stage_n = amt_of_stages;
    new_lv.stages = stages;
    new_lv.payload = payload_mass;
    return new_lv;
}

struct LV create_new_LV() {
    struct LV new_lv;
    printf("Name of Launcher: ");
    scanf(" %s", &new_lv.name);
    

    printf("Number of Stages: ");
    scanf(" %d", &new_lv.stage_n);

    for(int i = 0; i < new_lv.stage_n; i++) {
        printf("Stage %d:\n", i+1);
        printf("F_vac: ");
        scanf(" %lg", &new_lv.stages[i].F_vac);
        printf("F_sl: ");
        scanf(" %lg", &new_lv.stages[i].F_sl);
        printf("m0: ");
        scanf(" %lg", &new_lv.stages[i].m0);
        printf("me: ");
        scanf(" %lg", &new_lv.stages[i].me);
        printf("burn rate: ");
        scanf(" %lg", &new_lv.stages[i].burn_rate);
    }

    return new_lv;
}

void write_LV_to_file(struct LV lv) {
    char filename[42];  // 30 for the name, 9 for the diractory and 3 for .lv
    sprintf(filename, "Profiles/%s.lv", lv.name);

    // -------------------

    FILE *file;
    file = fopen(filename,"w");

    fprintf(file,"Stages: %d\n\n", lv.stage_n);

    for(int i = 0; i < lv.stage_n; i++) {
        fprintf(file,"Stage: %d:\n", i+1);
        fprintf(file,"\tF_vac: %g\n", lv.stages[i].F_vac);
        fprintf(file,"\tF_sl: %g\n", lv.stages[i].F_sl);
        fprintf(file,"\tm0: %g\n", lv.stages[i].m0);
        fprintf(file,"\tme: %g\n", lv.stages[i].me);
        fprintf(file,"\tburn rate: %g\n", lv.stages[i].burn_rate);
    }

    fclose(file);
}

struct LV read_LV_from_file(char * lv_name) {
    struct LV new_lv;
    new_lv.name = lv_name;

    char filename[42];  // 30 for the name, 9 for the diractory and 3 for .lv
    sprintf(filename, "Profiles/%s.lv", lv_name);

    // -------------------

    FILE *file;
    file = fopen(filename,"r");

    fscanf(file,"Stages: %d\n\n", &new_lv.stage_n);
    for(int i = 0; i < new_lv.stage_n; i++) {
        int temp;
        fscanf(file,"Stage: %d:\n", &temp);
        fscanf(file,"\tF_vac: %lg\n", &new_lv.stages[i].F_vac);
        fscanf(file,"\tF_sl: %lg\n", &new_lv.stages[i].F_sl);
        fscanf(file,"\tm0: %lg\n", &new_lv.stages[i].m0);
        fscanf(file,"\tme: %lg\n", &new_lv.stages[i].me);
        fscanf(file,"\tburn rate: %lg\n", &new_lv.stages[i].burn_rate);
    }

    fclose(file);

    return new_lv;
}