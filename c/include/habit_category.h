#ifndef HABIT_CATEGORY_H
#define HABIT_CATEGORY_H

#include "types.h"
#include "habit.h"

#define CATEGORY_HEALTH     "Health"
#define CATEGORY_EXERCISE   "Exercise"
#define CATEGORY_SCHOOL     "School"
#define CATEGORY_READING    "Reading"
#define CATEGORY_FINANCE    "Finance"
#define CATEGORY_PERSONAL   "Personal"
#define CATEGORY_GENERAL    "General"

/*
 * habit_set_category
 *
 * Sets the category of a habit.
 *
 * Parameters:
 *   habit: pointer to the habit (must be non-NULL)
 *   category: category string (must be non-NULL)
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if either parameter is NULL or category is too long
 */
Status habit_set_category(Habit *habit, const char *category);

/*
 * habit_get_category
 *
 * Gets the category of a habit.
 *
 * Parameters:
 *   habit: pointer to the habit (must be non-NULL)
 *   category_out: buffer to write category into (must be non-NULL)
 *   category_out_size: size of the buffer
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if any parameter is NULL
 */
Status habit_get_category(const Habit *habit, char *category_out, int category_out_size);

Status get_habit_category(HabitTracker *tracker, int habit_id, char *category_out, unsigned long category_out_size);
Status set_habit_category(HabitTracker *tracker, int habit_id, const char *category);

/*
 * habit_search_by_name
 *
 * Searches for habits whose name contains the query string (case-insensitive).
 *
 * Parameters:
 *   tracker: the habit tracker instance (must be non-NULL)
 *   query: partial name to search for (must be non-NULL)
 *   ids_out: on success, receives a newly allocated array of matching habit IDs
 *   count_out: on success, receives the number of matching habits
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if any parameter is NULL
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   The caller must free() ids_out.
 */
Status habit_search_by_name(HabitTracker *tracker, const char *query, int **ids_out, int *count_out);

/*
 * habit_tracker_completed_today
 *
 * Counts how many habits have been checked in today.
 *
 * Parameters:
 *   tracker: the habit tracker instance (must be non-NULL)
 *   count_out: on success, receives the number of habits completed today
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if any parameter is NULL
 */
Status habit_tracker_completed_today(HabitTracker *tracker, int *count_out);

/*
 * edit_habit
 *
 * Updates the name and description of an existing habit.
 *
 * Parameters:
 *   tracker: the habit tracker instance (must be non-NULL)
 *   habit_id: the ID of the habit to edit
 *   new_name: new name string (must be non-NULL, within HABIT_NAME_LENGTH)
 *   new_desc: new description string (must be non-NULL, within HABIT_DESC_LENGTH)
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if any parameter is NULL or strings are too long
 *   HT_NO_SUCH_HABIT if habit_id not found
 */
Status edit_habit(HabitTracker *tracker, int habit_id, const char *new_name, const char *new_desc);

#endif /* HABIT_CATEGORY_H */