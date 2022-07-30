struct Stage {
    double F_vac;       // Thrust produced by the engines in a vacuum [N]
    double F_sl;        // Thrust produced by the engines at sea level [N]
    double m0;          // initial mass (mass at t0) [kg]
    double me;          // vessel mass without fuel [kg]
    double burn_rate;   // burn rate of all running engines combined [kg/s]
};


struct LV {
    char * name;             // name of the launch vehicle
    int stage_n;            // amount of stages of the launch vehicle
    double payload;         // payload mass [kg]
    struct Stage *stages;   // the stages of the launch vehicle
};

// initialize launch vehicle with its stages (without payload mass)
struct  LV init_LV(char * name, int amt_of_stages, struct Stage *stages);
// user creates new LV-profile by being asked the parameters
void create_new_Profile();
// write LV parameters to file
void    write_LV_to_file(struct LV lv);
// read LV parameters from file
struct LV read_LV_from_file(char * lv_name);