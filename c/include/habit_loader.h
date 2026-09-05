#ifndef HABIT_LOADER_H
#define HABIT_LOADER_H

#include "types.h"   /* Habit */
#include "habit.h"


typedef struct Tree Tree;

/* ============================================================
 * Habit Loader
 *
 * Responsibilities:
 *   • Consume datagen library output (generator-owned memory)
 *   • Deep-copy habit data into student-owned Habit structs
 *   • Build a Tree from habits based on their IDs
 *
 * The loader bridges the gap between datagen's temporary output
 * and the student's persistent data structures.
 *
 * IMPORTANT:
 *   All datagen pointers are temporary.
 *   The loader performs deep copies so that habit data remains
 *   valid after datagen is shut down.
 * ============================================================ */

/* ============================================================
 * loader_load_habits
 * ============================================================
 *
 * Load habits by:
 *   1. Reading generated habit JSON from disk
 *   2. Parsing habit entries into Habit structs
 *   3. Building a tree of Habit structs and returning it to the tracker
 *
 * Parameters:
 *   config_file:
 *     Path to the generated JSON file.
 *
 * Outputs:
 *   tree_habit_out:
 *     On success, receives a pointer to the tree
 *
 *   num_habits_out:
 *     On success, receives the total number of habits loaded.
 *
 * Returns:
 *   OK on success
 *   DL_ERR_CONFIG if the config path is invalid
 *   DL_ERR_DATAGEN if datagen fails
 *   NO_MEMORY on allocation failure
 *
 * Ownership:
 *   - The caller owns the returned Tree.
 *   - All Habit structs are owned by the Tree as payloads.
 *   - Destroying the Tree frees all Habits.
 */
Status loader_load_habits(const char *config_file,
                         Tree **tree_habit_out,
                         int  *num_habits_out);

/*
 * New loader API: loads habits via datagen session started from config_file.
 * first_habit_out receives a deep-copied first generated habit when non-NULL.
 */
Status load_habits(const char *config_file,
                  Tree **tree_habit_out,
                  Habit **first_habit_out,
                  int *num_habits_out);


#endif /* HABIT_LOADER_H */
