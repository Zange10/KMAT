#include <ctype.h>

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