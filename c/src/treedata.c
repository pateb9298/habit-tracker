#include <stdio.h>
#include <stdlib.h>
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include "treedata.h"
/******************************************* 
 * Tree Data Functions for Strings. Use it as an example
*/
/**
 * Compares two string pointers using strcmp.
 * Returns:
 *   < 0 if a < b
 *   = 0 if a == b
 *   > 0 if a > b
 */
int compare_strings(void *a, void *b) {
    if (!a || !b) return 0;
    return strcmp((char *)a, (char *)b);
}

/**
 * Frees a string pointer.
 */
void destroy_string(void *data) {
    free(data);
}

/**
 * Returns a deep copy of the string (using strdup).
 */
void *copy_string(void *data) {
    if (!data) return NULL;
    return strdup((char *)data);
}

/**
 * Prints a string to stdout followed by a newline.
 */
void print_string(void *data) {
    if (data) {
        printf("%s\n", (char *)data);
    }
}