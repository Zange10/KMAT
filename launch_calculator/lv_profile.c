#include <stdio.h>
#include <stdlib.h>

#include "lv_profile.h"

struct LV init_LV(char * name, int amt_of_stages, struct Stage *stages) {
    struct LV new_lv;
	sprintf(new_lv.name, "%s", name);
    new_lv.stage_n = amt_of_stages;
    new_lv.stages = stages;
    return new_lv;
}

void print_LV(struct LV *lv) {
	printf("%s\n", lv->name);
	printf("A: %f\n", lv->A);
	printf("c_d: %f\n", lv->c_d);
	printf("Stages: %d\n", lv->stage_n);
	printf("LaunchProfile: %d\n", lv->lp_id);
	printf("ProfileParameters: %f, %f, %f, %f, %f\n\n",
		   lv->lp_params[0], lv->lp_params[1], lv->lp_params[2], lv->lp_params[3], lv->lp_params[4]);

	for(int i = 0; i < lv->stage_n; i++) {
		printf("Stage %d:\n", lv->stages[i].stage_id);
		printf("\tF_vac: %.2f kN\n", lv->stages[i].F_vac/1000);
		printf("\tF_sl: %.2f kN\n", lv->stages[i].F_sl/1000);
		printf("\tm0: %.3f t\n", lv->stages[i].m0/1000);
		printf("\tme: %.3f t\n", lv->stages[i].me/1000);
		printf("\tburn rate: %.2f kg/s\n", lv->stages[i].burn_rate);
	}
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
        fprintf(file,"\nStage %d:\n", i+1);
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
	sprintf(lv -> name, "%s", profile_name);

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
        fscanf(file,"Stage %d:\n", NULL);
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
	sprintf(lv -> name, "%s", profile_name);

    char filename[42];  // 30 for the name, 9 for the directory and 3 for .lv
    sprintf(filename, "Profiles/%s.lv", profile_name);

    // -------------------

    FILE *file;
    file = fopen(filename,"r");


    fscanf(file,"A: %lg\n", &lv->A);
    fscanf(file,"c_d: %lg\n", &lv->c_d);
    fscanf(file,"Stages: %d\n", &lv->stage_n);
	fscanf(file,"LaunchProfile: %d\n", &lv->lp_id);
	fscanf(file,"ProfileParameters: %lg, %lg, %lg, %lg, %lg\n\n",
		   &lv->lp_params[0], &lv->lp_params[1], &lv->lp_params[2], &lv->lp_params[3], &lv->lp_params[4]);

    lv->stages = (struct Stage*) calloc(lv->stage_n,sizeof(struct Stage));
    for(int i = 0; i < lv->stage_n; i++) {
        fscanf(file,"Stage %d:\n", &lv->stages[i].stage_id);
        fscanf(file,"\tF_vac: %lg\n", &lv->stages[i].F_vac);
        fscanf(file,"\tF_sl: %lg\n", &lv->stages[i].F_sl);
        fscanf(file,"\tm0: %lg\n", &lv->stages[i].m0);
        fscanf(file,"\tme: %lg\n", &lv->stages[i].me);
        fscanf(file,"\tburn rate: %lg\n", &lv->stages[i].burn_rate);
    }

    fclose(file);
}