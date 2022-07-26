//#define GRAVITATIONAL_CONSTANT 6.6743E-11   // [m³kg⁻¹s⁻²]

struct Body {
    double mu;          // gravitational parameter of body [m³/s²]
    double radius;      // radius of body [m]
    double sl_atmo_p;   // atmospheric pressure at sea level [Pa]
    double atmo_alt;    // highest altitude with atmosphere [m]
};

struct Body * VENUS() {
    struct Body venus;
    venus.mu = 3.2486e14;
    venus.radius = 1737000;
    venus.sl_atmo_p = 9321900;
    venus.atmo_alt = 145000;
    return &venus;
}

struct Body * EARTH() {
    struct Body earth;
    earth.mu = 3.9860e14;
    earth.radius = 6371000;
    earth.sl_atmo_p = 101325;
    earth.atmo_alt = 140000;
    return &earth;
}

struct Body * MOON() {
    struct Body moon;
    moon.mu = 0.0490e14;
    moon.radius = 1737000;
    moon.sl_atmo_p = 0;
    moon.atmo_alt = 0;
    return &moon;
}

struct Body * MARS() {
    struct Body mars;
    mars.mu = 0.42828e14;
    mars.radius = 3388000;
    mars.sl_atmo_p = 645;
    mars.atmo_alt = 125000;
    return &mars;
}

struct Body * JUPITER() {
    struct Body jupiter;
    jupiter.mu = 1266.87e14;
    jupiter.radius = 70000000;
    jupiter.sl_atmo_p = 101325;
    jupiter.atmo_alt = 140000;
    return &jupiter;
}

struct Body * SATURN() {
    struct Body saturn;
    saturn.mu = 379.31e14;
    saturn.radius = 58000000;
    saturn.sl_atmo_p = 101325;
    saturn.atmo_alt = 140000;
    return &saturn;
}