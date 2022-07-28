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

struct LV create_new_LV();

void write_LV_to_file(struct LV lv);