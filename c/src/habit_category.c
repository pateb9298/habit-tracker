#include <string.h>
#include "habit_category.h"
#include <stdlib.h>
#include <ctype.h>
#include "habit_tracker.h"
#include "tree.h"
#include <time.h>


Status habit_set_category(Habit *habit, const char *category) {
    if (habit == NULL || category == NULL) {
        return INVALID_ARGUMENT;
    }
    if (strlen(category) >= HABIT_CATEGORY_LENGTH) {
        return INVALID_ARGUMENT;
    }
    strncpy(habit->category, category, HABIT_CATEGORY_LENGTH - 1);
    habit->category[HABIT_CATEGORY_LENGTH - 1] = '\0';
    return OK;
}

Status habit_get_category(const Habit *habit, char *category_out, int category_out_size) {
    if (habit == NULL || category_out == NULL) {
        return INVALID_ARGUMENT;
    }
    strncpy(category_out, habit->category, category_out_size - 1);
    category_out[category_out_size - 1] = '\0';
    return OK;
}

Status habit_search_by_name(HabitTracker *tracker, const char *query, int **ids_out, int *count_out) {
    if (tracker == NULL || query == NULL || ids_out == NULL || count_out == NULL) {
        return INVALID_ARGUMENT;
    }

    *ids_out = NULL;
    *count_out = 0;

    // Allocate array big enough to hold all habit ids
    int *ids = malloc(sizeof(int) * tracker->habit_count);
    if (ids == NULL) {
        return NO_MEMORY;
    }

    int found = 0;

    // In-order traversal of tree to check each habit 
    TreeNode *stack[1000];
    int top = -1;
    TreeNode *current = get_root(tracker->tree);

    while (current != NULL || top >= 0) {
        while (current != NULL) {
            stack[++top] = current;
            current = get_left_child(current);
        }
        current = stack[top--];

        Habit *habit = get_node_data(current);
        if (habit != NULL) {
            // Case-insensitive search using lowercase comparison 
            char lower_name[HABIT_NAME_LENGTH];
            char lower_query[HABIT_NAME_LENGTH];

            // Copy and lowercase the habit name
            int i = 0;
            while (habit->name[i] && i < HABIT_NAME_LENGTH - 1) {
                lower_name[i] = (char)tolower((unsigned char)habit->name[i]);
                i++;
            }
            lower_name[i] = '\0';

            // Copy and lowercase the query
            i = 0;
            while (query[i] && i < HABIT_NAME_LENGTH - 1) {
                lower_query[i] = (char)tolower((unsigned char)query[i]);
                i++;
            }
            lower_query[i] = '\0';

            // If query is found anywhere in the name, add to results
            if (strstr(lower_name, lower_query) != NULL) {
                ids[found++] = habit->id;
            }
        }
        current = get_right_child(current);
    }

    *ids_out = ids;
    *count_out = found;
    return OK;
}

Status habit_tracker_completed_today(HabitTracker *tracker, int *count_out) {
    if (tracker == NULL || count_out == NULL) {
        return INVALID_ARGUMENT;
    }

    *count_out = 0;

    // Get today's date
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int today_year = t->tm_year + 1900;
    int today_month = t->tm_mon + 1;
    int today_day = t->tm_mday;

    int found = 0;

    // In-order traversal of tree
    TreeNode *stack[1000];
    int top = -1;
    TreeNode *current = get_root(tracker->tree);

    while (current != NULL || top >= 0) {
        while (current != NULL) {
            stack[++top] = current;
            current = get_left_child(current);
        }
        current = stack[top--];

        Habit *habit = get_node_data(current);
        if (habit != NULL) {
            // Check if last_checkin_day matches today 
            if (habit->last_checkin_day.year == today_year &&
                habit->last_checkin_day.month == today_month &&
                habit->last_checkin_day.day == today_day) {
                found++;
            }
        }
        current = get_right_child(current);
    }

    *count_out = found;
    return OK;
}

Status edit_habit(HabitTracker *tracker, int habit_id, const char *new_name, const char *new_desc) {
    // Check all parameters are valid 
    if (tracker == NULL || new_name == NULL || new_desc == NULL) {
        return INVALID_ARGUMENT;
    }
    // Check strings are not too long 
    if (strlen(new_name) >= HABIT_NAME_LENGTH || strlen(new_desc) >= HABIT_DESC_LENGTH) {
        return INVALID_ARGUMENT;
    }
    // Find the habit in the tree
    Habit key = {0};
    key.id = habit_id;
    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL) {
        return HT_NO_SUCH_HABIT;
    }
    // Get the habit data 
    Habit *habit = get_node_data(node);
    if (habit == NULL) {
        return INTERNAL_ERROR;
    }
    // Update the name 
    strncpy(habit->name, new_name, HABIT_NAME_LENGTH - 1);
    habit->name[HABIT_NAME_LENGTH - 1] = '\0';
    // Update the description 
    strncpy(habit->description, new_desc, HABIT_DESC_LENGTH - 1);
    habit->description[HABIT_DESC_LENGTH - 1] = '\0';
    return OK;
}