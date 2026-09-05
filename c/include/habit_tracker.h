#ifndef HABIT_TRACKER_H
#define HABIT_TRACKER_H

#include "types.h"   /* Status */

typedef struct Tree Tree;


/* ============================================================
 * HabitTracker - Main Habit Tracker Controller
 *
 * The HabitTracker orchestrates the entire program state:
 *   • Owns the connectivity Tree (habit hierarchy)
 *   • Indirectly owns all Habits via the Tree payloads
 *   • Implements CRUD operations on habits and the habit hierarchy
 *
 * All internal state is opaque to Python.
 * Only this public API is exposed via ctypes in later assignments.
 *
 * The Tree and Habit structures are never exposed directly.
 * ============================================================ */

/* ============================================================
 * Opaque HabitTracker type
 *
 * Python receives a pointer to this structure but cannot
 * inspect or modify its contents.
 * ============================================================ */

 typedef struct HabitTracker {
    Tree   *tree;               /* Owns all Habit payloads */
    int habit_count;             /* Cached number of habits */
    char   *config_file_path;    /* Reload source for reset */
} HabitTracker;


/* ============================================================
 * Creation & Destruction
 * ============================================================ */

/*
 * habit_tracker_create
 * ------------------
 * Build the entire tracker state by:
 *   1. Loading the habits using the habit loader
 *   2. Creating a tree of habits
 *   3. Caching the total habit count
 *
 * Parameters:
 *   config_path:
 *     Path to the configuration file (must not be NULL).
 *
 *   tracker_out:
 *     On success, receives an owning pointer to a newly created HabitTracker.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are NULL
 *   DL_ERR_DATAGEN if world loading fails
 *   NO_MEMORY on allocation failure
 *
 * Postconditions:
 *   On success, caller owns the engine and must call game_engine_destroy().
 */
Status habit_tracker_create(const char *config_file_path, HabitTracker **tracker_out);

/* ============================================================
 * Habit Queries
 * ============================================================ */

/*
 * Attempt to add the habit.
 *
 * Habit logic:
 *   - New habits are added as active by default
 *   - Habits can be archived but not deleted (for historical purposes)
 *   - Check-ins can be added only for existing habits (new habits start with no check-ins)
 *   - Check-ins can be added for any date (past or present)
 *   - Habit status is determined by a combination of the habit's status field and its check-in history
 *   - Habit streaks are calculated based on consecutive check-ins up to the current date
 *   - Habit IDs are unique integers assigned at creation and are used for all future references to that habit
 * Preconditions:
 *  tracker:    The habit tracker instance (must be non-NULL)
 *  habit_name: The name of the habit to add (must be non-NULL, max length HABIT_NAME_LENGTH)
 *  habit_desc: The description of the habit to add (must be non-NULL, max
 *  length HABIT_DESC_LENGTH)
 *  habit_date_created: The date the habit was created (must be a valid Date)
 * 
 * Parameters:
 *  tracker:    The habit tracker instance (must be non-NULL)
 *  habit_name: The name of the habit to add (must be non-NULL, max length HABIT_NAME_LENGTH)
 *  habit_desc: The description of the habit to add (must be non-NULL, max length HABIT_DESC_LENGTH)
 *  habit_date_created: The date the habit was created (must be a valid Date)
 *  habit_id: On success, receives the ID of the newly added habit
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   
 *   INTERNAL_ERROR on invariant failure
 */
Status add_habit(HabitTracker *tracker, const char *habit_name, const char *habit_desc, Date habit_date_created, int *habit_id_out);
/*
 * Attempt to check in for the habit.
 *
 * Parameters:
 *  tracker: The habit tracker instance (must be non-NULL)
 *  habit_id: The ID of the habit to check in for (must be a valid habit ID)
 *  date: The date of the check-in (must be a valid Date)
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on invariant failure
 */
Status check_in_habit(HabitTracker *tracker, int habit_id, const Date *date);
/*
 * Attempt to archive the habit.
 *
 * Parameters:
 *  tracker: The habit tracker instance (must be non-NULL)
 *  habit_id: The ID of the habit to archive (must be a valid habit ID)
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on invariant failure
 */
Status archive_habit(HabitTracker *tracker, int habit_id);
/*
 * Retrieve the status of a habit.
 *
 * Parameters:
 *  tracker: The habit tracker instance (must be non-NULL)
 *  habit_id: The ID of the habit (must be a valid habit ID)
 *  status_out: On success, receives the status of the habit
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on invariant failure
 */

/* ============================================================
 * Metadata Queries
 * ============================================================ */
Status get_habit_status(HabitTracker *tracker, int habit_id, HabitStatus *status_out);
/*
 * Retrieve the streak information for a habit.
 *
 * Parameters:
 *  tracker: The habit tracker instance (must be non-NULL)
 *  habit_id: The ID of the habit (must be a valid habit ID)
 *  current_streak_out: On success, receives the current streak length
 *  best_streak_out: On success, receives the best streak length
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on invariant failure
 */
Status get_habit_streaks(HabitTracker *tracker, int habit_id, int *current_streak_out, int *best_streak_out);
/*
 * Retrieve the check-in count for a habit.
 *
 * Parameters:
 *  tracker: The habit tracker instance (must be non-NULL)
 *  habit_id: The ID of the habit (must be a valid habit ID)
 *  checkin_count_out: On success, receives the check-in count
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on invariant failure
 */
Status get_habit_checkin_count(HabitTracker *tracker, int habit_id, int *checkin_count_out);
/*
 * Retrieve the date of the last check-in for a habit.
 *
 * Parameters:
 *  tracker: The habit tracker instance (must be non-NULL)
 *  habit_id: The ID of the habit (must be a valid habit ID)
 *  last_checkin_day_out: On success, receives the date of the last check-in
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on invariant failure
 */
Status get_habit_last_checkin_day(HabitTracker *tracker, int habit_id, Date *last_checkin_day_out);

/*
 * Retrieve the name of a habit.
 *
 * Parameters:
 *  tracker: The habit tracker instance (must be non-NULL)
 *  habit_id: The ID of the habit (must be a valid habit ID)
 *  name_out: On success, receives the name of the habit
 *  name_out_size: The size of the name_out buffer
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on invariant failure
 */
Status get_habit_name(HabitTracker *tracker, int habit_id, char *name_out, unsigned long name_out_size);
/*
 * Retrieve the description of a habit.
 *
 * Parameters:
 *  tracker: The habit tracker instance (must be non-NULL)
 *  habit_id: The ID of the habit (must be a valid habit ID)
 *  desc_out: On success, receives the description of the habit
 *  desc_out_size: The size of the desc_out buffer
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if inputs are invalid
 *   INTERNAL_ERROR on invariant failure
 */
Status get_habit_description(HabitTracker *tracker, int habit_id, char *desc_out, unsigned long desc_out_size);


/* ============================================================
 * Resetting / Restarting
 * ============================================================ */

/*
 * Reset the tracker to its initial state.
 *
 * Effects:
 *   • Habits are reloaded from the config file, replacing all existing state
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if tracker is NULL
 *   INTERNAL_ERROR if reset cannot complete
 */
Status habit_tracker_reset(HabitTracker *tracker);

/* ============================================================
 * Habits Rendering
 * ============================================================ */

/*
 * Render the all habits ordered by ID.
 *
 * The output includes:
 *   • habit name, 
 *   • habit description
 *   • habit status (active, inactive, archived)
 *   • current streak
 *   • best streak
 *   • total check-ins
 *   • date of last check-in
 * for all habits ordered by ID. 
 *
 * Concrete render format:
 *   Habits are rendered in ascending habit ID order.
 *   Each habit is rendered as exactly 8 lines using these labels:
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
 *   When rendering all habits, each rendered habit block is separated by a
 *   single blank line. There must be no trailing spaces on any line.
 *   The returned string must end with a final newline.
 *
 * Parameters:
 *   tracker:
 *     The habit tracker.
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
Status habit_tracker_render_all_habits(HabitTracker *tracker,
                                  char **str_out);

/*
 * Render the selected habit based on its ID.
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
 *   The selected habit is rendered as exactly 8 lines using these labels:
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
 *   tracker:
 *     The habit tracker.
 *   habit_id:
 *     The ID of the habit to render (must be a valid habit ID).
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
Status habit_tracker_render_habit(HabitTracker *tracker, int habit_id,
                                  char **str_out);


/*
 * Retrieve all habit IDs in the loaded data.
 *
 * Returns:
 *   OK on success (ids_out and count_out are set)
 *   INVALID_ARGUMENT if tracker is NULL
 *   NULL_POINTER if ids_out or count_out are NULL
 *   INTERNAL_ERROR if graph is invalid
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   The caller owns the returned array and must free() it.
 */
Status habit_tracker_get_habit_ids(const HabitTracker *tracker,
                                  int **ids_out,
                                  int *count_out);


/*
 * Destroy a habit tracker instance and free owned resources.
 */
void habit_tracker_destroy(HabitTracker *tracker);
/*
 * Free a string allocated by the habit tracker.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if ptr is NULL
 *   INTERNAL_ERROR if ptr is not a valid allocated string
 */
Status habit_tracker_free_string(char *ptr);

/*
 * Free an array allocated by the habit tracker.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if ptr is NULL
 *   INTERNAL_ERROR if ptr is not a valid allocated array
 */
Status habit_tracker_free_array(void *ptr);

#endif /* HABIT_TRACKER_H */