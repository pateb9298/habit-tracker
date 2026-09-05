#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "habit.h"

static int date_to_days(Date d)
{
    // initalized zero for tm struct (all fields are 0)
    //  tm defined in time.h
    struct tm t = {0};

    // tm_year is years since 1900
    t.tm_year = d.year - 1900;

    // tm_mon is 0 indexed, so subtract 1 to convert
    t.tm_mon = d.month - 1;

    // num of days (1 indexed anyways)
    t.tm_mday = d.day;

    // had to set ranodmly
    t.tm_hour = 12;

    // tells mktime to figure out DST itself
    t.tm_isdst = -1;

    // converts the tm struct into seconds since 1970-1-1
    time_t seconds = mktime(&t);

    // get day count
    return (int)(seconds / 86400);
}

Habit *create_habit(int id, const char *name, const char *description)
{
    Habit *h1 = malloc(sizeof(Habit));

    if (h1 == NULL)
    {
        return NULL;
    }

    if (name == NULL || description == NULL ||
        strlen(name) >= HABIT_NAME_LENGTH || strlen(description) >= HABIT_DESC_LENGTH)
    {
        free(h1);
        return NULL;
    }

    h1->id = id;
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy)
    strcpy(h1->name, name);
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy)
    strcpy(h1->description, description);

    strncpy(h1->category, "General", HABIT_CATEGORY_LENGTH - 1);
    h1->category[HABIT_CATEGORY_LENGTH - 1] = '\0';

    h1->total_checkins = 0;
    h1->is_archived = false;

    h1->created_date = (Date){0};
    h1->last_checkin_day = (Date){0};

    h1->records = NULL;
    return h1;
}

/* Frees all memory associated with a habit, including its completion records. */
void destroy_habit(Habit *habit)
{

    if (habit == NULL)
    {
        return;
    }

    CompletionRecord *temp = NULL;
    CompletionRecord *current = habit->records;

    while (current != NULL)
    {

        temp = current->next;
        free(current);
        current = temp;
    }

    free(habit);
}

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

CompletionRecordStatus check_in(Habit *habit, const Date *date)
{
    if (habit == NULL)
    {
        return CR_ERR_INVALID_HABIT;
    }

    if (date == NULL || !is_valid_date(date))
    {
        return CR_ERR_INVALID_DATE;
    }

    CompletionRecord *current = habit->records;
    while (current != NULL)
    {

        if (current->date.year == date->year && current->date.month == date->month && current->date.day == date->day)
        {
            current->completed = true;
            habit->last_checkin_day = *date;

            return CR_ERR_OK;
        }
        current = current->next;
    }

    CompletionRecord *new_record = malloc(sizeof(CompletionRecord));

    if (new_record == NULL)
    {
        return CR_ERR_INVALID_HABIT;
    }
    new_record->completed = true;
    new_record->date = *date;
    habit->total_checkins += 1;
    habit->last_checkin_day = *date;

    new_record->next = habit->records;
    habit->records = new_record;

    return CR_ERR_OK;
}

HabitStatus habit_get_status(const Habit *habit, int recent_threshold_days, Date current_date)
{

    if (habit == NULL)
    {
        return HABIT_INACTIVE;
    }

    if (habit->is_archived)
    {
        return HABIT_ARCHIVED;
    }
    int today_days = date_to_days(current_date);
    int last_checkin_days = date_to_days(habit->last_checkin_day);

    if (today_days - last_checkin_days <= recent_threshold_days)
    {
        return HABIT_ACTIVE;
    }

    return HABIT_INACTIVE;
}

int current_streak(Habit *habit)
{

    if (habit == NULL || habit->records == NULL)
    {
        return 0;
    }

    int count = 0;
    CompletionRecord *curr = habit->records;

    // Counting the num of records
    while (curr != NULL)
    {
        count++;
        curr = curr->next;
    }

    // copying the linked list into arrays to sort by date
    Date *dates = malloc(sizeof(Date) * count);
    bool *completed = malloc(sizeof(bool) * count);

    // memory check
    if (dates == NULL || completed == NULL)
    {
        free(dates);
        free(completed);
        return 0;
    }

    // copying the data into arrays
    curr = habit->records;
    for (int i = 0; i < count; i++)
    {
        dates[i] = curr->date;
        completed[i] = curr->completed;
        curr = curr->next;
    }

    // Sorting both arrays by date using bubble sort
    for (int a_one = 0; a_one < count - 1; a_one++)
    {
        for (int b_two = 0; b_two < count - a_one - 1; b_two++)
        {
            if (date_to_days(dates[b_two]) > date_to_days(dates[b_two + 1]))
            {

                // doing the swapss
                Date tmp = dates[b_two];
                dates[b_two] = dates[b_two + 1];
                dates[b_two + 1] = tmp;

                bool tb = completed[b_two];
                completed[b_two] = completed[b_two + 1];
                completed[b_two + 1] = tb;
            }
        }
    }

    // walk backwards from the most recent record, counting the streak
    int i = count - 1;

    // If the most recent record is not completed -> the streak is 0
    if (!completed[i])
    {
        free(dates);
        free(completed);
        return 0;
    }

    int streak = 1;

    // Walk backwards through the sorted records to count consecutive completed days
    while (i > 0)
    {
        int curr_day = date_to_days(dates[i]);
        int prev_day = date_to_days(dates[i - 1]);

        // If the previous day is not completed then break the streak
        if (!completed[i - 1])
        {
            break;
        }

        // If the previous day is exactly one day before the current day, increase  streak by on2
        if (curr_day - prev_day == 1)
        {
            streak++;
            i--;
        }
        else
        {
            break;
        }
    }

    free(dates);
    free(completed);

    return streak;
}

// how recent is recent???
bool is_active(Habit *h)
{
    if (h == NULL)
    {
        return false;
    }
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    Date today = {t->tm_year + 1900, t->tm_mon + 1, t->tm_mday};

    return habit_get_status(h, 7, today) == HABIT_ACTIVE;
}

//-- kind of like that max problem
int calculate_best_streak(Habit *habit)
{
    if (habit == NULL || habit->records == NULL)
    {
        return 0;
    }

    int count = 0;
    CompletionRecord *curr = habit->records;

    while (curr != NULL)
    {
        count++;
        curr = curr->next;
    }

    Date *dates = malloc(sizeof(Date) * count);
    bool *completed = malloc(sizeof(bool) * count);

    if (dates == NULL || completed == NULL)
    {
        free(dates);
        free(completed);
        return 0;
    }

    curr = habit->records;
    int i = 0;
    while (curr != NULL)
    {
        dates[i] = curr->date;
        completed[i] = curr->completed;
        i++;
        curr = curr->next;
    }

    // Bubble sort
    for (int a_one = 0; a_one < count - 1; a_one++)
    {
        for (int b_two = 0; b_two < count - a_one - 1; b_two++)
        {
            if (date_to_days(dates[b_two]) > date_to_days(dates[b_two + 1]))
            {

                Date tmp_date = dates[b_two];
                dates[b_two] = dates[b_two + 1];
                dates[b_two + 1] = tmp_date;

                bool tmp_bool = completed[b_two];
                completed[b_two] = completed[b_two + 1];
                completed[b_two + 1] = tmp_bool;
            }
        }
    }

    int best = 0;
    int current = 0;

    for (int j = 0; j < count; j++)
    {

        if (!completed[j])
        {
            current = 0;
            continue;
        }

        if (j > 0)
        {
            int prev_day = date_to_days(dates[j - 1]);
            int curr_day = date_to_days(dates[j]);

            if (completed[j - 1] && (curr_day - prev_day == 1))
            {
                current++;
            }
            else
            {
                current = 1;
            }
        }
        else
        {
            current = 1;
        }

        if (current > best)
        {
            best = current;
        }
    }

    free(dates);
    free(completed);

    return best;
}

Status habit_render_habit(Habit *habit, char **str_out)
{
    if (habit == NULL || str_out == NULL)
    {
        return INVALID_ARGUMENT;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    Date today = {t->tm_year + 1900, t->tm_mon + 1, t->tm_mday};

    HabitStatus status = habit_get_status(habit, 7, today);

    const char *status_str = NULL;
    if (status == HABIT_ACTIVE)
    {
        status_str = "active";
    }
    else if (status == HABIT_INACTIVE)
    {
        status_str = "inactive";
    }
    else if (status == HABIT_ARCHIVED)
    {
        status_str = "archived";
    }
    else
    {
        status_str = "invalid";
    }

    int curr_streak = current_streak(habit);
    int best_streak = calculate_best_streak(habit);

    char date_str[11];
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)sprintf(date_str, "%04d-%02d-%02d",
                  habit->last_checkin_day.year,
                  habit->last_checkin_day.month,
                  habit->last_checkin_day.day);

    char *result = malloc(1024);
    if (result == NULL)
    {
        return INTERNAL_ERROR;
    }
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)snprintf(
        result, 1024,
        "ID: %d\n"
        "Name: %s\n"
        "Description: %s\n"
        "Status: %s\n"
        "Current streak: %d\n"
        "Best streak: %d\n"
        "Total check-ins: %d\n"
        "Last check-in: %s\n",
        habit->id,
        habit->name,
        habit->description,
        status_str,
        curr_streak,
        best_streak,
        habit->total_checkins,
        date_str);

    *str_out = result;
    return OK;
}
