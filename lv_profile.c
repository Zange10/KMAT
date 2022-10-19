#include <stdio.h>
#include <stdlib.h>

#include "lv_profile.h"

struct LV init_LV(char * name, int amt_of_stages, struct Stage *stages) {
    struct LV new_lv;
    new_lv.name = name;
    new_lv.stage_n = amt_of_stages;
    new_lv.stages = stages;
    return new_lv;
}

void create_new_Profile() {
    char name[30];
    int amt_of_stages;

    printf("Name of Launcher: ");
    scanf("%s", name);

    printf("Number of Stages: ");
    scanf("%d", &amt_of_stages);

    struct Stage stages[amt_of_stages];

    for(int i = 0; i < amt_of_stages; i++) {
        printf("Stage %d:\n", i+1);
        printf("F_vac: ");
        scanf("%lg", &stages[i].F_vac);
        printf("F_sl: ");
        scanf("%lg", &stages[i].F_sl);
        printf("m0: ");
        scanf("%lg", &stages[i].m0);
        printf("me: ");
        scanf("%lg", &stages[i].me);
        printf("burn rate: ");
        scanf("%lg", &stages[i].burn_rate);
    }
    struct LV lv = init_LV(name, amt_of_stages, stages);
    write_LV_to_file(lv);
}

void write_LV_to_file(struct LV lv) {
    char filename[42];  // 30 for the name, 9 for the diractory and 3 for .lv
    sprintf(filename, "Profiles/%s.lv", lv.name);

    // -------------------

    FILE *file;
    file = fopen(filename,"w");

    fprintf(file,"Stages: %d\n", lv.stage_n);

    for(int i = 0; i < lv.stage_n; i++) {
        fprintf(file,"\nStage: %d:\n", i+1);
        fprintf(file,"\tF_vac: %g\n", lv.stages[i].F_vac);
        fprintf(file,"\tF_sl: %g\n", lv.stages[i].F_sl);
        fprintf(file,"\tm0: %g\n", lv.stages[i].m0);
        fprintf(file,"\tme: %g\n", lv.stages[i].me);
        fprintf(file,"\tburn rate: %g", lv.stages[i].burn_rate);
    }

    fclose(file);
}

void read_LV_from_file(struct LV * lv) {
    char profile_name[30];
    printf("Profile: ");
    scanf("%s", profile_name);
    lv -> name = profile_name;

    char filename[42];  // 30 for the name, 9 for the diractory and 3 for .lv
    sprintf(filename, "Profiles/%s.lv", profile_name);

    // -------------------

    FILE *file;
    file = fopen(filename,"r");


    fscanf(file,"Stages: %d\n\n", &lv->stage_n);
    lv->stages = (struct Stage*) calloc(lv->stage_n,sizeof(struct Stage));

    for(int i = 0; i < lv->stage_n; i++) {
        int temp;
        fscanf(file,"Stage: %d:\n", &temp);
        fscanf(file,"\tF_vac: %lg\n", &lv->stages[i].F_vac);
        fscanf(file,"\tF_sl: %lg\n", &lv->stages[i].F_sl);
        fscanf(file,"\tm0: %lg\n", &lv->stages[i].m0);
        fscanf(file,"\tme: %lg\n", &lv->stages[i].me);
        fscanf(file,"\tburn rate: %lg\n", &lv->stages[i].burn_rate);
    }

    fclose(file);
}

void get_test_LV(struct LV * lv) {
    char profile_name[] = "tester";
    lv -> name = profile_name;

    char filename[42];  // 30 for the name, 9 for the diractory and 3 for .lv
    sprintf(filename, "Profiles/%s.lv", profile_name);

    // -------------------

    FILE *file;
    file = fopen(filename,"r");


    fscanf(file,"Stages: %d\n\n", &lv->stage_n);
    lv->stages = (struct Stage*) calloc(lv->stage_n,sizeof(struct Stage));

    for(int i = 0; i < lv->stage_n; i++) {
        int temp;
        fscanf(file,"Stage: %d:\n", &temp);
        fscanf(file,"\tF_vac: %lg\n", &lv->stages[i].F_vac);
        fscanf(file,"\tF_sl: %lg\n", &lv->stages[i].F_sl);
        fscanf(file,"\tm0: %lg\n", &lv->stages[i].m0);
        fscanf(file,"\tme: %lg\n", &lv->stages[i].me);
        fscanf(file,"\tburn rate: %lg\n", &lv->stages[i].burn_rate);
    }

    fclose(file);
}