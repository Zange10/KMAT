struct Stage {
    double F_vac;       // Thrust produced by the engines in a vacuum [N]
    double F_sl;        // Thrust produced by the engines at sea level [N]
    double m0;          // initial mass (mass at t0) [kg]
    double me;          // vessel mass without fuel [kg]
    double burn_rate;   // burn rate of all running engines combined [kg/s]
};


struct LV {
    char *name;             // name of the launch vehicle
    int stage_n;            // amount of stages of the launch vehicle
    double payload;         // payload mass [kg]
    struct Stage *stages;   // the stages of the launch vehicle
};

// initialize stage (F_sl [kN], F_vac [kN], m0 [t], me [t], br [kg/s])
struct  Stage init_stage(double F_sl, double F_vac, double m0, double me, double br);
// initialize launch vehicle with its stages and payload mass
struct  LV init_LV(char * name, int amt_of_stages, struct Stage *stages, int payload_mass);
// user creates new LV by being asked the parameters
struct  LV create_new_LV();
// write LV parameters to file
void    write_LV_to_file(struct LV lv);
// read LV parameters from file
struct LV read_LV_from_file(char * lv_name);