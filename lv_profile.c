#include <stdio.h>

#include "lv_profile.h"


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