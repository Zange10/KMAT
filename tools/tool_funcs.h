#ifndef TOOL_FUNCS
#define TOOL_FUNCS

#include <ctype.h>
#include <sys/time.h>

typedef struct TimingMeasurement {
	struct TimingMeasurement *next;
	double elapsed_time;
	char name[256];
} TimingMeasurement;

typedef struct TimingMeasurements {
	struct timeval start, end;
	TimingMeasurement *first;
} TimingMeasurements;

/**
 * @brief Initialise timing measurements
 *
 * @return Timing measurements struct
 */
TimingMeasurements init_timing_measurements();

/**
 * @brief Start a timing measurement
 *
 * @param tm The pointer to the timing measurements holder
 */
void start_time_measurement(TimingMeasurements *tm);

/**
 * @brief Get the total measurement time (sum of all measurements)
 *
 * @param tm The timing measurements holder
 *
 * @return The total measured time
 */
double get_total_timing_time(TimingMeasurements tm);

/**
 * @brief Print an overview of time measurements
 *
 * @param tm The timing measurements holder
 */
void print_timing_measurements(TimingMeasurements tm);

/**
 * @brief End a timing measurement and store it in its holder
 *
 * @param tm The timing measurements holder
 * @param name Name to be displayed with measurement when printed
 */
void end_time_measurement(TimingMeasurements *tm, char *name);

/**
 * @brief Get the last measured time
 *
 * @param tm The timing measurements holder
 *
 * @return The last entry into the timing measurement holder
 */
TimingMeasurement *get_last_timing_measurement(TimingMeasurements tm);

/**
 * @brief Free all stored measurements
 *
 * @param tm The timing measurements holder
 */
void free_timing_measurements(TimingMeasurements *tm);



/**
 * @brief let user select a given option by index (integer)
 *
 * @param title The string, which should be placed at the top of the request
 * @param options The different choosable options (separated by semicolon)
 * @param question The string, which should be placed at the bottom of the request and to which the user should answer
 *
 * @return The index of the option, the user selected
 */
int user_selection(char *title, char *options, char *question);


/**
 * @brief prints x amount of underscores and two additional line-breaks
 *
 * @param x The amount of underscores to be printed
 */
void print_separator(int x);


/**
 * @brief Compares to strings as to whether they are the same or not (not case-sensitive)
 *
 * @param a String 1
 * @param b String 2
 *
 * @return Returns 1 if the strings are the same and 0 if not
 */
int strcicmp(char const *a, char const *b);


/**
 * @brief show progress in command line in percent
 *
 * @param text The descriptive text that is displayed in front of the progress status
 * @param progress The amount of progress that has been made
 * @param total The total progress needed for 100%
 */
void show_progress(char *text, double progress, double total);


#endif