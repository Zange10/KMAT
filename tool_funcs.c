#include <stdio.h>
#include "tool_funcs.h"

int user_selection(char *title, char *options, char *question) {
    printf("\n\n%s\n", title);
    print_separator(49);
    int i = 1;  // 1, because see while loop
    int j = 0;

    while(options[i-1] != 0) {  // -1, because i is the first character of the option (character in front is relevant)
        if(options[0] == ' ') options = &options[i+1];      // skip first character if space

        while(options[i] != 0 && options[i] != ';') i++;

        printf("| - % 2d: %.*s", j, i, options);
        for(int k = 0; k < (47-(i+8))/8; k++) printf("\t");     // for a max line length of 32 (next line for initial \t)
        printf("\t|\n");

        options = &options[i+1];
        i = 0;
        j++;
    }

    print_separator(49);
    printf("\n%s", question);

    int sel;
    scanf(" %d", &sel);
    printf("\n");
    print_separator(49);
    print_separator(49);
    printf("\n");
    return sel;
}


void print_separator(int x) {
    for(int i = 0; i < x; i++) printf("_");
    printf("\n\n");
}