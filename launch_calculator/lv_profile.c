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

void write_temp_LV_file() {
    char name[30];
    int amt_of_stages;

    printf("Name of Launcher: ");
    scanf("%s", name);

    printf("Number of Stages: ");
    scanf("%d", &amt_of_stages);

    char filename[42];  // 30 for the name, 9 for the directory and 3 for .lv
    sprintf(filename, "./Profiles/%s.lv", name);

    // -------------------
    printf("%s\n", filename);
    FILE *file;
    file = fopen(filename,"w+");

    if (file == NULL) {
        printf("Failed to open the file.\n");
        return;
    }

    fprintf(file,"A: 1\n");
    fprintf(file,"c_d: 0.7\n", amt_of_stages);
    fprintf(file,"Stages: %d\n", amt_of_stages);

    for(int i = 0; i < amt_of_stages; i++) {
        fprintf(file,"\nStage: %d:\n", i+1);
        fprintf(file,"\tF_vac: 1000\n");
        fprintf(file,"\tF_sl: 1000\n");
        fprintf(file,"\tm0: 1000\n");
        fprintf(file,"\tme: 50\n");
        fprintf(file,"\tburn rate: 10");
    }

    fclose(file);
}

void read_LV_from_file(struct LV * lv) {
    char profile_name[30];
    printf("Profile: ");
    scanf("%s", profile_name);
    lv -> name = profile_name;

    char filename[42];  // 30 for the name, 9 for the directory and 3 for .lv
    sprintf(filename, "Profiles/%s.lv", profile_name);

    // -------------------

    FILE *file;
    file = fopen(filename,"r");


    fscanf(file,"A: %lg\n", &lv->A);
    fscanf(file,"c_d: %lg\n", &lv->c_d);
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


    fscanf(file,"A: %lg\n", &lv->A);
    fscanf(file,"c_d: %lg\n", &lv->c_d);
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