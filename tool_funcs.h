#include <ctype.h>
// let user select a given option by index (integer); title, options and question as string with options separarted by ';'
int user_selection(char *title, char *options, char *question);
// prints x amount of underscores and two additional line-breaks
void print_separator(int x);
// returns 1 if strings are not the same, returns 0 if strings are the same
int strcicmp(char const *a, char const *b);

void show_progress(char *text, double progress, double total);