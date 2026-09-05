#include "habit_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "habit.h"
#include "habit_generator.h"
#include "tree.h"

// Compare two habits by their IDs
// Function pointer passed to create_tree as the compare function
// Compares 2 habits by id so the tree knows whether to go left or right
static int compare_habits_by_id(void *a, void *b)
{
    const Habit *lhs = (const Habit *)a;
    const Habit *rhs = (const Habit *)b;

    if (lhs == NULL || rhs == NULL)
    {
        return 0;
    }

    if (lhs->id < rhs->id)
    {
        return -1;
    }
    if (lhs->id > rhs->id)
    {
        return 1;
    }

    return 0;
}

// Destroy a habit payload, including its linked list of completion records
// Function pointer passed to create_tree as the destroy function
// Called automatically by destroy Tree on every node
static void destroy_habit_payload(void *data)
{
    Habit *habit = (Habit *)data;
    CompletionRecord *record = NULL;

    if (habit == NULL)
    {
        return;
    }

    record = habit->records;
    while (record != NULL)
    {
        CompletionRecord *next = record->next;
        free(record);
        record = next;
    }

    free(habit);
}

// Deep copy a linked list of completion records
static CompletionRecord *copy_record_list(const CompletionRecord *src_head)
{
    CompletionRecord *dst_head = NULL;
    CompletionRecord *dst_tail = NULL;

    while (src_head != NULL)
    {
        CompletionRecord *node = (CompletionRecord *)malloc(sizeof(CompletionRecord));
        if (node == NULL)
        {
            while (dst_head != NULL)
            {
                CompletionRecord *next = dst_head->next;
                free(dst_head);
                dst_head = next;
            }
            return NULL;
        }

        *node = *src_head;
        node->next = NULL;

        if (dst_head == NULL)
        {
            dst_head = node;
            dst_tail = node;
        }
        else
        {
            dst_tail->next = node;
            dst_tail = node;
        }

        src_head = src_head->next;
    }

    return dst_head;
}

// Deep copy a Habit payload, including its linked list of completion records
static void *copy_habit_payload(void *data)
{
    Habit *src = (Habit *)data;
    Habit *dst = NULL;

    if (src == NULL)
    {
        return NULL;
    }

    dst = (Habit *)malloc(sizeof(Habit));
    if (dst == NULL)
    {
        return NULL;
    }

    *dst = *src;
    dst->records = copy_record_list(src->records);
    if (src->records != NULL && dst->records == NULL)
    {
        free(dst);
        return NULL;
    }

    return dst;
}

// NOLINTNEXTLINE(cert-err34-c)
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
// Parse JSON habits from the file
// reads a JSON file produced by writeHabitstoJSon using fgets and sscanf
// returns the array and count using out parameters --> caller owns array
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static Status parse_json_habits(const char *json_path, Habit **habits_out, int *count_out)
{
    // NOLINTBEGIN(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    // NOLINTBEGIN(cert-err34-c)
    FILE *f = NULL;
    char line[1024];
    Habit *habits = NULL;
    int expected_count = -1;
    int current_index = -1;

    if (json_path == NULL || habits_out == NULL || count_out == NULL)
    {

        return INVALID_ARGUMENT;
    }

    *habits_out = NULL;
    *count_out = 0;

    f = fopen(json_path, "r");
    if (f == NULL)
    {
        return DL_ERR_CONFIG;
    }

    while (fgets(line, sizeof(line), f) != NULL)
    {
        int parsed_int = 0;

        if (sscanf(line, " \"habit_count\" : %d", &parsed_int) == 1 ||
            sscanf(line, " \"habit_count\": %d", &parsed_int) == 1)
        {
            expected_count = parsed_int;
            if (expected_count < 0)
            {
                (void)fclose(f);
                // HABIT_LOG_POINT("loader_parse_json_negative_count");
                free(habits);
                return DL_ERR_DATAGEN;
            }
            if (expected_count > 0)
            {
                free(habits);
                habits = (Habit *)calloc((size_t)expected_count, sizeof(Habit));
                if (habits == NULL)
                {
                    (void)fclose(f);
                    // HABIT_LOG_POINT("loader_parse_json_alloc_fail");
                    return NO_MEMORY;
                }
            }
            continue;
        }

        if (expected_count <= 0 || habits == NULL)
        {
            continue;
        }

        if ((sscanf(line, " \"id\" : %d", &parsed_int) == 1 ||
             sscanf(line, " \"id\": %d", &parsed_int) == 1) &&
            current_index + 1 < expected_count)
        {
            current_index++;
            habits[current_index].id = parsed_int;
            habits[current_index].id = parsed_int;
            continue;
        }

        if (current_index < 0 || current_index >= expected_count)
        {
            continue;
        }

        if (sscanf(line, " \"name\" : \"%99[^\"]\"", habits[current_index].name) == 1 ||
            sscanf(line, " \"name\": \"%99[^\"]\"", habits[current_index].name) == 1)
        {
            continue;
        }

        if (sscanf(line, " \"description\" : \"%499[^\"]\"", habits[current_index].description) == 1 ||
            sscanf(line, " \"description\": \"%499[^\"]\"", habits[current_index].description) == 1)
        {
            continue;
        }

        if (sscanf(line, " \"total_checkins\" : %d", &parsed_int) == 1 ||
            sscanf(line, " \"total_checkins\": %d", &parsed_int) == 1)
        {
            habits[current_index].total_checkins = parsed_int;
            continue;
        }

        if (sscanf(line, " \"is_archived\" : %d", &parsed_int) == 1 ||
            sscanf(line, " \"is_archived\": %d", &parsed_int) == 1)
        {
            habits[current_index].is_archived = (parsed_int != 0);
            continue;
        }

        if (sscanf(line, " \"created_date\" : \"%d-%d-%d\"",
                   &habits[current_index].created_date.year,
                   &habits[current_index].created_date.month,
                   &habits[current_index].created_date.day) == 3 ||
            sscanf(line, " \"created_date\": \"%d-%d-%d\"",
                   &habits[current_index].created_date.year,
                   &habits[current_index].created_date.month,
                   &habits[current_index].created_date.day) == 3)
        {
            continue;
        }

        if (sscanf(line, " \"last_checkin_day\" : \"%d-%d-%d\"",
                   &habits[current_index].last_checkin_day.year,
                   &habits[current_index].last_checkin_day.month,
                   &habits[current_index].last_checkin_day.day) == 3 ||
            sscanf(line, " \"last_checkin_day\": \"%d-%d-%d\"",
                   &habits[current_index].last_checkin_day.year,
                   &habits[current_index].last_checkin_day.month,
                   &habits[current_index].last_checkin_day.day) == 3)
        {
            continue;
        }
    }

    (void)fclose(f);

    if (expected_count < 0)
    {
        free(habits);
        // HABIT_LOG_POINT("loader_parse_json_missing_count");
        return DL_ERR_DATAGEN;
    }

    *habits_out = habits;
    *count_out = expected_count;
    // HABIT_LOG_POINT("loader_parse_json_ok");
    return OK;
    // NOLINTEND(cert-err34-c)
    // NOLINTEND(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
}

// the newer loader (i dont use) uses the datagen sesion API instead of reading a JSON file
// calls start_datagen to generate habits from .ini config file
// calls stop_datagen() when done
Status load_habits(const char *config_file,
                   Tree **tree_habit_out,
                   Habit **first_habit_out,
                   int *num_habits_out)
{
    Tree *tree = NULL;
    const Habit *generated = NULL;
    HabitGenStatus gen_status = HABIT_GEN_OK;
    int processed = 0;
    // HABIT_LOG_POINT("loader_load_habits_enter");

    if (tree_habit_out == NULL || num_habits_out == NULL)
    {
        // HABIT_LOG_POINT("loader_load_habits_invalid_out");
        return INVALID_ARGUMENT;
    }
    if (config_file == NULL)
    {
        // HABIT_LOG_POINT("loader_load_habits_invalid_config");
        return INVALID_ARGUMENT;
    }

    *tree_habit_out = NULL;
    if (first_habit_out != NULL)
    {
        *first_habit_out = NULL;
    }
    *num_habits_out = 0;

    gen_status = start_datagen(config_file);

    if (gen_status != HABIT_GEN_OK)
    {
        // HABIT_LOG_POINT("loader_load_habits_datagen_fail");
        if (gen_status == HABIT_GEN_ERR_ALLOC)
        {
            return NO_MEMORY;
        }
        if (gen_status == HABIT_GEN_ERR_IO || gen_status == HABIT_GEN_ERR_INVALID_ARG)
        {
            return DL_ERR_CONFIG;
        }
        return DL_ERR_DATAGEN;
    }

    tree = create_tree(compare_habits_by_id, destroy_habit_payload, copy_habit_payload);
    if (tree == NULL)
    {
        stop_datagen();
        // HABIT_LOG_POINT("loader_load_habits_tree_alloc_fail");
        return NO_MEMORY;
    }

    reset_datagen_iterator();
    while ((generated = next_generated_habit()) != NULL)
    {
        if (first_habit_out != NULL && *first_habit_out == NULL)
        {
            *first_habit_out = (Habit *)copy_habit_payload((void *)generated);
            if (*first_habit_out == NULL)
            {
                destroy_tree(tree);
                stop_datagen();
                // HABIT_LOG_POINT("loader_load_habits_first_copy_fail");
                return NO_MEMORY;
            }
        }

        if (insert_node(tree, (void *)generated) != 0)
        {
            if (first_habit_out != NULL && *first_habit_out != NULL)
            {
                destroy_habit_payload(*first_habit_out);
                *first_habit_out = NULL;
            }
            destroy_tree(tree);
            stop_datagen();
            // HABIT_LOG_POINT("loader_load_habits_insert_fail");
            return NO_MEMORY;
        }

        processed++;
    }

    stop_datagen();
    *tree_habit_out = tree;
    *num_habits_out = processed;
    // HABIT_LOG_POINT("loader_load_habits_ok");
    return OK;
}

// the older loader --> reads habits from a pre-existing JSON file on disk
// calls parse_json_habits to get an array of Habit structs
// then isnerts each one into a new tree using insertNode
// caller owns the returned tree --> destroy_tree frees all habits inside
Status loader_load_habits(const char *config_file,
                          Tree **tree_habit_out,
                          int *num_habits_out)
{
    Habit *loaded = NULL;
    int count = 0;
    int i = 0;
    Tree *tree = NULL;
    Status st = OK;
    // HABIT_LOG_POINT("loader_loader_load_enter");

    if (tree_habit_out == NULL || num_habits_out == NULL)
    {
        // HABIT_LOG_POINT("loader_loader_load_invalid_arg");
        return INVALID_ARGUMENT;
    }

    *tree_habit_out = NULL;
    *num_habits_out = 0;

    st = parse_json_habits(config_file, &loaded, &count);
    if (st != OK)
    {
        // HABIT_LOG_POINT("loader_loader_load_parse_fail");
        return st;
    }

    tree = create_tree(compare_habits_by_id, destroy_habit_payload, copy_habit_payload);
    if (tree == NULL)
    {
        free(loaded);
        // HABIT_LOG_POINT("loader_loader_load_tree_alloc_fail");
        return NO_MEMORY;
    }

    for (i = 0; i < count; i++)
    {
        if (insert_node(tree, &loaded[i]) != 0)
        {
            destroy_tree(tree);
            free(loaded);
            // HABIT_LOG_POINT("loader_loader_load_insert_fail");
            return NO_MEMORY;
        }
    }

    free(loaded);
    *tree_habit_out = tree;
    *num_habits_out = count;
    // HABIT_LOG_POINT("loader_loader_load_ok");
    return OK;
}
