
#ifndef HABIT_H
#define HABIT_H

#include "types.h"

/* ============================================================
 * Habit
 *
 * Stores all information associated with a single habit.
 *
 * Each habit has a unique identifier, descriptive metadata,
 * and a linked list containing completion history.
 *
 * NOTE: Completion records are dynamically allocated and
 * must be released when a habit is destroyed.
 * ============================================================ */

typedef struct Habit{

    int id;

    char name[HABIT_NAME_LENGTH];
    char description[HABIT_DESC_LENGTH];
    char category[HABIT_CATEGORY_LENGTH];
    int total_checkins;
    bool is_archived;

    Date created_date;
    Date last_checkin_day;

    CompletionRecord* records;

} Habit;

typedef struct HabitTracker HabitTracker;

/* ============================================================
 * Habit Operations
 *
 * These functions manage habit creation, destruction, and analysis.
 * They are used internally by the HabitTracker and are not exposed
 * directly to Python.
 * ============================================================ */

 /* Creates a new habit with the given ID, name, and description.
  * Returns a pointer to the newly created Habit, or NULL on failure.
  * The caller is responsible for freeing the returned Habit using destroy_habit.
  * 
  * Parameters: 
  *     id: unique identifier for the habit
  *     name: descriptive name of the habit (must be non-NULL and within length limits)
  *     description: detailed description of the habit (must be non-NULL and within length limits)
  * 
  * Preconditions: 
  *     id: unique habit_id, 
  *     name: name and must be non-NULL and within length limits,
  *     description: description must be non-NULL and within length limits.
  * 
  * Postconditions:
  *     On success, returns a pointer to a newly allocated Habit with the specified fields set and an empty completion record list. 
  *     On failure, returns NULL. 
  * 
  * Returns:
  *    Pointer to the newly created Habit on success
  *    NULL on failure (e.g., invalid parameters or memory allocation failure)  
  * 
  */
 Habit *create_habit(int id, const char *name, const char *description);

/* Frees all memory associated with a habit, including its completion records. */
void destroy_habit(Habit *habit);

/*checkIn
Marks a habit as completed for a specific date. If a record for that date already exists, it updates the completion status to true. If no record exists for that date, it creates a new completion record with completed set to true and adds it to the habit's linked list of records.
Parameters:
    habit: Pointer to the Habit to check in (must be non-NULL)
    date: Pointer to the Date of the check-in (must be non-NULL)
Preconditions:
    habit: must be a valid pointer to an existing Habit
    date: must be a valid pointer to a Date struct
Postconditions:
    If a completion record for the specified date already exists, its completed field is set to true.
    If no completion record exists for the specified date, a new record is created with the date set to the specified date 
    and completed set to true, and this record is added to the habit's linked list of completion records and total_checkins is incremented.
    In both cases the habit's last_checkin_day field is updated to the specified date.
Returns:
    CR_ERR_OK on success
    CR_ERR_INVALID_DATE if the date is invalid
    CR_ERR_INVALID_HABIT if the habit pointer is NULL
*/
CompletionRecordStatus check_in(Habit *habit, const Date *date);

/* ============================================================
 * Habit Analysis Functions
 * ============================================================ */

/* getHabitStatus
 *
 * Determines the current status of a habit based on its fields and completion history.
 *
 * Returns:
 *   HABIT_ACTIVE if the habit is active (status is HABIT_ACTIVE and has a recent (within last recent_threshold_days days) check-in)
 *   HABIT_INACTIVE if the habit is inactive (status is HABIT_ACTIVE but has no recent check-ins)
 *   HABIT_ARCHIVED if the habit is archived (status is HABIT_ARCHIVED and is not active)
 *   HABIT_INVALID_STATUS if the habit pointer is NULL or if the habit's status field contains an invalid value
 * 
 */
HabitStatus habit_get_status(const Habit *habit, int recent_threshold_days, Date current_date);

/*
currentStreak
Calculates the current streak of consecutive completed days for a habit.
Returns the count of consecutive days up to and including the most recent check-in.
*/
int current_streak(Habit *habit);

/*
 * isActive
 *
 * Determines if a habit is currently active based on its status and recent activity.
 *
 * Returns:
 *   true if the habit is active (status is HABIT_ACTIVE and has a recent check-in)
 *   false otherwise
 */
bool is_active(Habit *h);

/* calculateBestStreak
 *
 * Calculates the longest streak of consecutive completed days in the habit's history.
 *
 * Returns:
 *   The length of the longest streak of consecutive completed days.
 */
int calculate_best_streak(Habit *habit);

/*
 * Render the habit.
 *
 * The output includes:
 *   • habit name, 
 *   • habit description
 *   • habit status (active, inactive, archived)
 *   • current streak
 *   • best streak
 *   • total check-ins
 *   • date of last check-in
 * 
 *
 * Concrete render format:
 *   The habit is rendered as exactly 8 lines using these labels:
 *
 *     ID: <id>\n
 *     Name: <name>\n
 *     Description: <description>\n
 *     Status: <active|inactive|archived>\n
 *     Current streak: <current_streak>\n
 *     Best streak: <best_streak>\n
 *     Total check-ins: <total_checkins>\n
 *     Last check-in: <YYYY-MM-DD>\n
 *
 *   There must be no trailing spaces on any line.
 *   The returned string must end with a final newline.
 *
 * Parameters:
 *   habit:
 *     The habit to render.
 *   str_out:
 *     On success, receives a newly allocated string.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on rendering failure
 *
 * Ownership:
 *   The caller must free() the returned string.
 */
Status habit_render_habit(Habit* habit, char **str_out);

/*
 * Render completion records.
 *
 * The output includes every completion record in the linked list starting at
 * checkins, rendered in traversal order beginning with the first node pointed
 * to by checkins.
 *
 * Concrete render format:
 *   Each completion record is rendered as exactly 2 lines using these labels:
 *
 *     Date: <YYYY-MM-DD>\n
 *     Completed: <true|false>\n
 *
 *   Each rendered record block is separated by a single blank line.
 *   There must be no trailing spaces on any line.
 *   The returned string must end with a final newline when at least one
 *   record is rendered.
 *   If checkins is NULL, the function returns OK and sets str_out to a newly
 *   allocated empty string.
 *
 * Parameters:
 *   checkins:
 *     Head of the completion record linked list to render.
 *   str_out:
 *     On success, receives a newly allocated string.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if str_out is NULL
 *   INTERNAL_ERROR on rendering failure
 *
 * Ownership:
 *   The caller must free() the returned string.
 */
Status habit_render_checkins(HabitTracker *tracker, int habit_id, char **str_out);


#endif /* HABIT_H */