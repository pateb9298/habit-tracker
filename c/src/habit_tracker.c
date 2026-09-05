#include "habit_tracker.h"
#include "habit.h"
#include "habit_category.h"
#include <time.h>
#include "habit_loader.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

Status habit_tracker_create(const char *config_file_path, HabitTracker **tracker_out)
{
    if (config_file_path == NULL || tracker_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    HabitTracker *tracker = malloc(sizeof(HabitTracker));
    if (tracker == NULL)
    {
        return NO_MEMORY;
    }

    size_t path_len = strlen(config_file_path);
    tracker->config_file_path = malloc(path_len + 1);
    if (tracker->config_file_path == NULL)
    {
        free(tracker);
        return NO_MEMORY;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy)
    strcpy(tracker->config_file_path, config_file_path);

    int num_habits = 0;
    // Load habits and build the tree (this is form habit_loader )
    Status status = load_habits(config_file_path, &tracker->tree, NULL, &num_habits);
    if (status != OK)
    {
        free(tracker->config_file_path);
        free(tracker);
        return status;
    }

    tracker->habit_count = num_habits;
    // receives an owning pointer to newly created HabitTracker
    *tracker_out = tracker;
    return OK;
}

void habit_tracker_destroy(HabitTracker *tracker)
{
    if (tracker != NULL)
    {
        destroy_tree(tracker->tree);
        free(tracker->config_file_path);
        free(tracker);
    }
}

/*
 * Free a string allocated by the habit tracker.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if ptr is NULL
 *   INTERNAL_ERROR if ptr is not a valid allocated string
 */
Status habit_tracker_free_string(char *ptr)
{
    if (ptr == NULL)
    {
        return INVALID_ARGUMENT;
    }
    free(ptr);
    return OK;
}

/*
 * Free an array allocated by the habit tracker.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if ptr is NULL
 *   INTERNAL_ERROR if ptr is not a valid allocated array
 */

// Array is one single, contiguous block of memory, created by one malloc/calloc call
// takes void ptr because it can be an array of any type (int, char*, etc.)
Status habit_tracker_free_array(void *ptr)
{
    if (ptr == NULL)
    {
        return INVALID_ARGUMENT;
    }
    free(ptr);
    return OK;
}

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

static bool is_valid_date(const Date *d)
{
    if (d == NULL)
        return false;

    if (d->year < 0)
        return false;
    if (d->month < 1 || d->month > 12)
        return false;
    if (d->day < 1 || d->day > 31)
        return false;

    return true;
}

static int get_max_habit_id(Tree *tree)
{
    if (tree == NULL)
    {
        return 0;
    }
    TreeNode *node = get_root(tree);
    if (node == NULL)
    {
        return 0;
    }
    while (get_right_child(node) != NULL)
    {
        node = get_right_child(node);
    }
    Habit *h = get_node_data(node);
    return (h != NULL) ? h->id : 0;
}

Status add_habit(HabitTracker *tracker, const char *habit_name, const char *habit_desc, const Date habit_date_created, int *habit_id_out)
{
    if (tracker == NULL || habit_name == NULL || habit_desc == NULL || habit_id_out == NULL)
    {
        return INVALID_ARGUMENT;
    }
    if (strlen(habit_name) >= HABIT_NAME_LENGTH || strlen(habit_desc) >= HABIT_DESC_LENGTH)
    {
        return INVALID_ARGUMENT;
    }
    if (!is_valid_date(&habit_date_created))
    {
        return INVALID_ARGUMENT;
    }

    int new_id = get_max_habit_id(tracker->tree) + 1;

    Habit *new_habit = create_habit(new_id, habit_name, habit_desc);
    if (new_habit == NULL)
    {
        return NO_MEMORY;
    }

    new_habit->created_date = habit_date_created;

    // insert the new habit into the tree
    int insert_status = insert_node(tracker->tree, new_habit);

    // something about this check is wrong???
    if (insert_status != 0)
    {
        destroy_habit(new_habit);
        return INTERNAL_ERROR;
    }

    tracker->habit_count++;

    // need to destroy because the tree makes a copy of the habit
    // so we don't want to leak memory
    destroy_habit(new_habit);
    *habit_id_out = new_id;
    return OK;
}

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
Status check_in_habit(HabitTracker *tracker, int habit_id, const Date *date)
{
    if (tracker == NULL || date == NULL)
    {
        return INVALID_ARGUMENT;
    }
    if (!is_valid_date(date))
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT; // Habit not found
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    CompletionRecordStatus cr_status = check_in(habit, date);
    // if all is valid it shoudl work but if not we return internal error
    if (cr_status != CR_ERR_OK)
    {
        return INTERNAL_ERROR;
    }

    return OK;
}

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

Status archive_habit(HabitTracker *tracker, int habit_id)
{
    if (tracker == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT;
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    habit->is_archived = true;
    return OK;
}

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

Status get_habit_status(HabitTracker *tracker, int habit_id, HabitStatus *status_out)
{
    if (tracker == NULL || status_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        // Am i allowed to return something that is not in the ???
        return HT_NO_SUCH_HABIT; // Habit not found
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    Date today;
    today.year = t->tm_year + 1900;
    today.month = t->tm_mon + 1;
    today.day = t->tm_mday;

    // I used 7 assumption enter in readme ig??
    *status_out = habit_get_status(habit, 7, today);
    return OK;
}

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
Status get_habit_streaks(HabitTracker *tracker, int habit_id, int *current_streak_out, int *best_streak_out)
{
    if (tracker == NULL || current_streak_out == NULL || best_streak_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT; // Habit not found
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    *current_streak_out = current_streak(habit);
    *best_streak_out = calculate_best_streak(habit);
    return OK;
}

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
Status get_habit_checkin_count(HabitTracker *tracker, int habit_id, int *checkin_count_out)
{
    if (tracker == NULL || checkin_count_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT; // Habit not found
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    *checkin_count_out = habit->total_checkins;
    return OK;
}

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
Status get_habit_last_checkin_day(HabitTracker *tracker, int habit_id, Date *last_checkin_day_out)
{
    if (tracker == NULL || last_checkin_day_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT;
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    *last_checkin_day_out = habit->last_checkin_day;
    return OK;
}

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
Status get_habit_name(HabitTracker *tracker, int habit_id, char *name_out, unsigned long name_out_size)
{
    if (tracker == NULL || tracker->tree == NULL || name_out == NULL || name_out_size == 0)
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    strncpy(name_out, habit->name, name_out_size - 1);
    name_out[name_out_size - 1] = '\0';

    return OK;
}

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
Status get_habit_description(HabitTracker *tracker, int habit_id, char *desc_out, unsigned long desc_out_size)
{
    if (tracker == NULL || tracker->tree == NULL ||
        desc_out == NULL || desc_out_size == 0)
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    strncpy(desc_out, habit->description, desc_out_size - 1);
    desc_out[desc_out_size - 1] = '\0';

    return OK;
}

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
Status habit_tracker_reset(HabitTracker *tracker)
{
    if (tracker == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Tree *old_tree = tracker->tree;

    Tree *new_tree = NULL;
    int new_count = 0;

    Status status = load_habits(tracker->config_file_path, &new_tree, NULL, &new_count);
    if (status != OK)
    {
        return INTERNAL_ERROR;
    }

    destroy_tree(old_tree);

    tracker->tree = new_tree;
    tracker->habit_count = new_count;

    return OK;
}

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
                                       char **str_out)
{
    // NOLINTBEGIN(clang-analyzer-security.insecureAPI.strcpy)
    if (tracker == NULL || str_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (tracker->tree == NULL)
    {
        return INTERNAL_ERROR;
    }

    // takes name + desc (def up to 600 chars + more content)
    char *result = malloc(tracker->habit_count * 1025 + 1);
    if (result == NULL)
    {
        return INTERNAL_ERROR;
    }

    // initialize the result string to an empty string
    result[0] = '\0';

    // creating a stack for in-order traversal
    // goes left to current to right
    // setting 1000 for now as the max habits??
    TreeNode *stack[1000];

    // stack is empty for now
    int top = -1;

    // start at the root of the tree
    // w ill probably go to the left most node first
    TreeNode *current = get_root(tracker->tree);

    // using this to track if this is the first habit being rendered,
    // so i dont add a blank line before the very first habit
    bool first = true;

    // keep going while there are nodes to visit OR
    // nodes still on stack

    // at the end if there is no rightchild then it skips these while loops
    // goes to the next in stack instead and then goes to right child
    while (current != NULL || top >= 0)
    {

        // go as far left as possible, and push the nodes along way
        // on to the stack
        while (current != NULL)
        {
            stack[++top] = current;
            current = get_left_child(current);
        }

        // pop the top node from the stack and visit it
        current = stack[top--];

        // get the Habit data stored inside
        Habit *habit = get_node_data(current);

        // render that specific habit data into a string
        char *habit_str = NULL;
        // using the render function from habit.c
        Status status = habit_render_habit(habit, &habit_str);

        // if render failed
        if (status != OK)
        {
            free(result);
            return status;
        }

        // add a blank line if not first habit
        if (!first)
        {
            strcat(result, "\n");
        }

        // append the rendered habit string to the result
        strcat(result, habit_str);

        // freeing the string to avoid meomry leak
        free(habit_str);

        // set this to false now so i have rendered one habit
        first = false;

        current = get_right_child(current);
    }

    *str_out = result;
    return OK;
    // NOLINTEND(clang-analyzer-security.insecureAPI.strcpy)
}

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
                                  char **str_out)
{
    if (tracker == NULL || str_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    if (tracker->tree == NULL)
    {
        return INTERNAL_ERROR;
    }

    // initializing output to NULL in case of early return
    *str_out = NULL;

    Habit key = {0};
    key.id = habit_id;

    // Find node in tree
    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT;
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    // Reuse existing render helper
    char *habit_str = NULL;
    Status status = habit_render_habit(habit, &habit_str);
    if (status != OK)
    {
        return status;
    }

    // Return result
    *str_out = habit_str;
    return OK;
}

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
                                   int *count_out)
{
    if (tracker == NULL)
    {
        return INVALID_ARGUMENT;
    }
    if (ids_out == NULL || count_out == NULL)
    {
        return NULL_POINTER;
    }
    if (tracker->tree == NULL)
    {
        return INTERNAL_ERROR;
    }

    // Initialize output parameters for safety incase return early
    *ids_out = NULL;
    *count_out = 0;

    // get num of habits from cached count
    int count = tracker->habit_count;

    // if not habits, return early
    if (count == 0)
    {
        *ids_out = NULL;
        *count_out = 0;
        return OK;
    }

    // allocate an array big enough to hold one integer per habit
    int *ids = malloc(sizeof(int) * count);
    if (ids == NULL)
    {
        return NO_MEMORY;
    }

    // in order traversal again
    // to get ascending habit_id order
    TreeNode *stack[1000];

    // empty stack for now
    int top = -1;

    // start at the root of the tree
    TreeNode *current = get_root(tracker->tree);

    // keeps tracks of whcih index in ids[] to fill next
    int index = 0;

    // keep going while nodes to visit or nodes still on stack
    while (current != NULL || top >= 0)
    {
        // go as far left as possibl + push nodes to stack
        while (current != NULL)
        {
            stack[++top] = current;
            current = get_left_child(current);
        }

        // pop next node to visit
        current = stack[top--];

        // get habit storedi n that node
        Habit *habit = get_node_data(current);
        // add the habit_id to the ids array if habit is not NULL
        if (habit != NULL)
        {
            ids[index++] = habit->id;
        }

        // now move to right subtree
        current = get_right_child(current);
    }

    // return results
    *ids_out = ids;
    // use index instead of count to know how many were actually stored
    // in case of NULL
    *count_out = index;

    return OK;
}

Status habit_render_checkins(HabitTracker *tracker, int habit_id, char **str_out)
{
    // NOLINTBEGIN(clang-analyzer-security.insecureAPI.strcpy)
    if (tracker == NULL || str_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    Habit key = {0};
    key.id = habit_id;

    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT;
    }

    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    CompletionRecord *checkins = habit->records;

    if (checkins == NULL)
    {
        *str_out = malloc(1);

        if (*str_out == NULL)
        {
            return INTERNAL_ERROR;
        }

        // derefrence -->  index 0 of the pointer to the string and set it to null terminator
        (*str_out)[0] = '\0';
        return OK;
    }

    int count = 0;
    CompletionRecord *curr = checkins;

    while (curr != NULL)
    {
        count++;
        curr = curr->next;
    }

    // size_t to represent the size of the string to be allocated
    // eacg record will have about 32 chars each so about 50 should be good
    size_t size = count * 50 + 1;

    char *result = malloc(size);
    if (result == NULL)
    {
        return INTERNAL_ERROR;
    }

    // for strcat to work properly, we need to initialize the first character of result to the null terminator
    result[0] = '\0';

    curr = checkins;

    while (curr != NULL)
    {
        char buffer[64];

        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        (void)snprintf(
            buffer,
            sizeof(buffer),
            "Date: %04d-%02d-%02d\n"
            "Completed: %s\n",
            curr->date.year,
            curr->date.month,
            curr->date.day,
            curr->completed ? "true" : "false");

        // append the buffer to the end of result string
        strcat(result, buffer);

        if (curr->next != NULL)
        {
            strcat(result, "\n");
        }

        curr = curr->next;
    }

    // assign the result to the output pointer
    *str_out = result;
    return OK;
    // NOLINTEND(clang-analyzer-security.insecureAPI.strcpy)
}

Status get_habit_category(HabitTracker *tracker, int habit_id, char *category_out, unsigned long category_out_size)
{
    if (tracker == NULL || category_out == NULL || category_out_size == 0)
    {
        return INVALID_ARGUMENT;
    }
    Habit key = {0};
    key.id = habit_id;
    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT;
    }
    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }

    // Copy the habit's category string INTO the output buffer (reading FROM habit struct)
    strncpy(category_out, habit->category, category_out_size - 1);

    category_out[category_out_size - 1] = '\0';
    return OK;
}

Status set_habit_category(HabitTracker *tracker, int habit_id, const char *category)
{
    if (tracker == NULL || category == NULL)
    {
        return INVALID_ARGUMENT;
    }
    if (strlen(category) >= HABIT_CATEGORY_LENGTH)
    {
        return INVALID_ARGUMENT;
    }
    Habit key = {0};
    key.id = habit_id;
    TreeNode *node = find_node(tracker->tree, &key);
    if (node == NULL)
    {
        return HT_NO_SUCH_HABIT;
    }
    Habit *habit = get_node_data(node);
    if (habit == NULL)
    {
        return INTERNAL_ERROR;
    }
    /* Copy the incoming category string INTO the habit's category field (writing TO habit struct) */
    strncpy(habit->category, category, HABIT_CATEGORY_LENGTH - 1);
    habit->category[HABIT_CATEGORY_LENGTH - 1] = '\0';
    return OK;
}