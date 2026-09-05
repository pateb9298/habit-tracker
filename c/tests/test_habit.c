#include <check.h>
#include <string.h>
#include "habit.h"
#include <time.h>
#include <stdlib.h>

// look over these ???
static Date date_from_offset(int day_offset)
{
    time_t now = time(NULL);
    time_t target = now + (time_t)day_offset * 86400;
    struct tm *t = localtime(&target);

    Date d;
    d.year = t->tm_year + 1900;
    d.month = t->tm_mon + 1;
    d.day = t->tm_mday;
    return d;
}

static Date today_date(void)
{
    return date_from_offset(0);
}

// create_habit Tests
START_TEST(test_create_habit_valid)
{
    Habit *h = create_habit(42, "Drink water", "Stay hydrated");

    ck_assert_ptr_nonnull(h);
    ck_assert_int_eq(h->id, 42);
    ck_assert_str_eq(h->name, "Drink water");
    ck_assert_str_eq(h->description, "Stay hydrated");
    ck_assert_int_eq(h->total_checkins, 0);
    ck_assert(h->is_archived == false);
    ck_assert_ptr_null(h->records);

    destroy_habit(h);
}
END_TEST

START_TEST(test_create_habit_null_name)
{
    Habit *h = create_habit(1, NULL, "desc");
    ck_assert_ptr_null(h);
}
END_TEST

START_TEST(test_create_habit_null_description)
{
    Habit *h = create_habit(1, "name", NULL);
    ck_assert_ptr_null(h);
}
END_TEST

// look over??
START_TEST(test_create_habit_name_too_long)
{
    char long_name[HABIT_NAME_LENGTH + 1];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';

    Habit *h = create_habit(1, long_name, "desc");
    ck_assert_ptr_null(h);
}
END_TEST

// look over??
START_TEST(test_create_habit_description_too_long)
{
    char long_desc[HABIT_DESC_LENGTH + 1];
    memset(long_desc, 'b', sizeof(long_desc) - 1);
    long_desc[sizeof(long_desc) - 1] = '\0';

    Habit *h = create_habit(1, "name", long_desc);
    ck_assert_ptr_null(h);
}
END_TEST

// destroy_habit tests
START_TEST(test_destroy_habit_null_does_not_crash)
{
    destroy_habit(NULL);

    ck_assert(true);
}
END_TEST

START_TEST(test_destroy_habit_frees_records)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d1 = {2024, 1, 1};
    Date d2 = {2024, 1, 2};
    Date d3 = {2024, 1, 3};

    ck_assert_int_eq(check_in(h, &d1), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d2), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d3), CR_ERR_OK);

    ck_assert_ptr_nonnull(h->records);

    destroy_habit(h);
    ck_assert(true);
}
END_TEST

// checkIn Tests

START_TEST(test_checkin_null_habit)
{
    Date d = {2024, 1, 1};
    ck_assert_int_eq(check_in(NULL, &d), CR_ERR_INVALID_HABIT);
}
END_TEST

START_TEST(test_checkin_null_date)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    ck_assert_int_eq(check_in(h, NULL), CR_ERR_INVALID_DATE);

    destroy_habit(h);
}
END_TEST

START_TEST(test_checkin_invalid_date_month)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date bad_low = {2024, 0, 1};
    Date bad_high = {2024, 13, 1};

    ck_assert_int_eq(check_in(h, &bad_low), CR_ERR_INVALID_DATE);
    ck_assert_int_eq(check_in(h, &bad_high), CR_ERR_INVALID_DATE);

    destroy_habit(h);
}
END_TEST

START_TEST(test_checkin_invalid_date_day)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date bad_low = {2024, 1, 0};
    Date bad_high = {2024, 1, 32};

    ck_assert_int_eq(check_in(h, &bad_low), CR_ERR_INVALID_DATE);
    ck_assert_int_eq(check_in(h, &bad_high), CR_ERR_INVALID_DATE);

    destroy_habit(h);
}
END_TEST

START_TEST(test_checkin_invalid_date_negative_year)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date bad = {-1, 1, 1};
    ck_assert_int_eq(check_in(h, &bad), CR_ERR_INVALID_DATE);

    destroy_habit(h);
}
END_TEST

// ANSWER WHY YOU DIDNT USE SETUP AND FIXTURE FOR THIS FILE??
START_TEST(test_checkin_new_record)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d = {2024, 1, 1};
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    ck_assert_int_eq(h->total_checkins, 1);
    ck_assert_ptr_nonnull(h->records);
    ck_assert(h->records->completed == true);
    ck_assert_int_eq(h->records->date.year, 2024);
    ck_assert_int_eq(h->records->date.month, 1);
    ck_assert_int_eq(h->records->date.day, 1);

    ck_assert_int_eq(h->last_checkin_day.year, 2024);
    ck_assert_int_eq(h->last_checkin_day.month, 1);
    ck_assert_int_eq(h->last_checkin_day.day, 1);

    destroy_habit(h);
}
END_TEST

START_TEST(test_checkin_same_date_twice_does_not_duplicate)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d = {2024, 1, 1};
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    ck_assert_int_eq(h->total_checkins, 1);

    int record_count = 0;
    for (CompletionRecord *c = h->records; c != NULL; c = c->next)
    {
        record_count++;
    }
    ck_assert_int_eq(record_count, 1);

    destroy_habit(h);
}
END_TEST

START_TEST(test_checkin_multiple_distinct_dates)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d1 = {2024, 1, 1};
    Date d2 = {2024, 1, 2};
    Date d3 = {2024, 1, 3};

    ck_assert_int_eq(check_in(h, &d1), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d2), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d3), CR_ERR_OK);

    ck_assert_int_eq(h->total_checkins, 3);

    /* most recent checkin date is tracked */
    ck_assert_int_eq(h->last_checkin_day.year, 2024);
    ck_assert_int_eq(h->last_checkin_day.month, 1);
    ck_assert_int_eq(h->last_checkin_day.day, 3);

    int record_count = 0;
    for (CompletionRecord *c = h->records; c != NULL; c = c->next)
    {
        record_count++;
    }
    ck_assert_int_eq(record_count, 3);

    destroy_habit(h);
}
END_TEST

// get_habit_status Tests

START_TEST(test_get_habit_status_null_habit)
{
    Date d = today_date();
    ck_assert_int_eq(habit_get_status(NULL, 7, d), HABIT_INACTIVE);
}
END_TEST

START_TEST(test_get_habit_status_archived)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    // today_date()??
    Date d = today_date();
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    h->is_archived = true;

    ck_assert_int_eq(habit_get_status(h, 7, d), HABIT_ARCHIVED);

    destroy_habit(h);
}
END_TEST

START_TEST(test_get_habit_status_active_with_recent_checkin)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d = today_date();
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    ck_assert_int_eq(habit_get_status(h, 7, d), HABIT_ACTIVE);

    destroy_habit(h);
}
END_TEST

START_TEST(test_get_habit_status_inactive_with_old_checkin)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    /* well outside the 7-day threshold */
    // date_from_offset??????
    Date d = date_from_offset(-100);
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    Date now = today_date();
    ck_assert_int_eq(habit_get_status(h, 7, now), HABIT_INACTIVE);

    destroy_habit(h);
}
END_TEST

START_TEST(test_get_habit_status_no_checkins_is_inactive)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d = today_date();
    ck_assert_int_eq(habit_get_status(h, 7, d), HABIT_INACTIVE);

    destroy_habit(h);
}
END_TEST

// current_streak tests
START_TEST(test_current_streak_null_habit)
{
    ck_assert_int_eq(current_streak(NULL), 0);
}
END_TEST

START_TEST(test_current_streak_no_records)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    ck_assert_int_eq(current_streak(h), 0);

    destroy_habit(h);
}
END_TEST

START_TEST(test_current_streak_single_completed_day)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d = {2024, 1, 1};
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    ck_assert_int_eq(current_streak(h), 1);

    destroy_habit(h);
}
END_TEST

START_TEST(test_current_streak_consecutive_days)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d1 = {2024, 1, 1};
    Date d2 = {2024, 1, 2};
    Date d3 = {2024, 1, 3};

    ck_assert_int_eq(check_in(h, &d2), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d1), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d3), CR_ERR_OK);

    ck_assert_int_eq(current_streak(h), 3);

    destroy_habit(h);
}
END_TEST

START_TEST(test_current_streak_breaks_on_gap)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d1 = {2024, 1, 1};
    Date d2 = {2024, 1, 2};
    Date d3 = {2024, 1, 10};

    ck_assert_int_eq(check_in(h, &d1), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d2), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d3), CR_ERR_OK);

    ck_assert_int_eq(current_streak(h), 1);

    destroy_habit(h);
}
END_TEST

// look over??
START_TEST(test_current_streak_zero_when_most_recent_incomplete)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d1 = {2024, 1, 1};
    Date d2 = {2024, 1, 2};

    ck_assert_int_eq(check_in(h, &d1), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d2), CR_ERR_OK);

    h->records->completed = false;
    ck_assert_int_eq(h->records->date.day, 2);

    ck_assert_int_eq(current_streak(h), 0);

    destroy_habit(h);
}
END_TEST

// is_active
START_TEST(test_is_active_null)
{
    ck_assert(is_active(NULL) == false);
}
END_TEST

START_TEST(test_is_active_true_for_recent_checkin)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    // today_date() - check over??
    Date d = today_date();
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    ck_assert(is_active(h) == true);

    destroy_habit(h);
}
END_TEST

START_TEST(test_is_active_false_for_old_checkin)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    // date_from_offset - look over??
    Date d = date_from_offset(-100);
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    ck_assert(is_active(h) == false);

    destroy_habit(h);
}
END_TEST

START_TEST(test_is_active_false_when_archived)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    // look over??
    Date d = today_date();
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);
    h->is_archived = true;

    ck_assert(is_active(h) == false);

    destroy_habit(h);
}
END_TEST

// calculate_best_streak Tests

START_TEST(test_best_streak_null_habit)
{
    ck_assert_int_eq(calculate_best_streak(NULL), 0);
}
END_TEST

START_TEST(test_best_streak_no_records)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    ck_assert_int_eq(calculate_best_streak(h), 0);

    destroy_habit(h);
}
END_TEST

START_TEST(test_best_streak_single_day)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d = {2024, 1, 1};
    ck_assert_int_eq(check_in(h, &d), CR_ERR_OK);

    ck_assert_int_eq(calculate_best_streak(h), 1);

    destroy_habit(h);
}
END_TEST

START_TEST(test_best_streak_finds_longest_past_streak)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date jan1 = {2024, 1, 1};
    Date jan2 = {2024, 1, 2};
    Date jan3 = {2024, 1, 3};
    Date jan4 = {2024, 1, 4};
    Date jan5 = {2024, 1, 5};

    Date jan10 = {2024, 1, 10};
    Date jan11 = {2024, 1, 11};

    ck_assert_int_eq(check_in(h, &jan1), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &jan2), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &jan3), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &jan4), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &jan5), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &jan10), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &jan11), CR_ERR_OK);

    ck_assert_int_eq(calculate_best_streak(h), 5);
    ck_assert_int_eq(current_streak(h), 2);

    destroy_habit(h);
}
END_TEST

START_TEST(test_best_streak_all_incomplete_is_zero)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date d1 = {2024, 1, 1};
    Date d2 = {2024, 1, 2};

    ck_assert_int_eq(check_in(h, &d1), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &d2), CR_ERR_OK);

    for (CompletionRecord *c = h->records; c != NULL; c = c->next)
    {
        c->completed = false;
    }

    ck_assert_int_eq(calculate_best_streak(h), 0);
    ck_assert_int_eq(current_streak(h), 0);

    destroy_habit(h);
}
END_TEST

// look over??
START_TEST(test_best_streak_resets_after_gap_and_incomplete_day)
{
    Habit *h = create_habit(1, "name", "desc");
    ck_assert_ptr_nonnull(h);

    Date jan1 = {2024, 1, 1};
    Date jan2 = {2024, 1, 2};
    Date jan3 = {2024, 1, 3};

    ck_assert_int_eq(check_in(h, &jan1), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &jan2), CR_ERR_OK);
    ck_assert_int_eq(check_in(h, &jan3), CR_ERR_OK);

    for (CompletionRecord *c = h->records; c != NULL; c = c->next)
    {
        if (c->date.day == 2)
        {
            c->completed = false;
        }
    }

    ck_assert_int_eq(calculate_best_streak(h), 1);

    destroy_habit(h);
}
END_TEST

Suite *habit_suite(void)
{
    Suite *s = suite_create("Habit");
    TCase *tc = tcase_create("Core");

    /* create_habit */
    tcase_add_test(tc, test_create_habit_valid);
    tcase_add_test(tc, test_create_habit_null_name);
    tcase_add_test(tc, test_create_habit_null_description);
    tcase_add_test(tc, test_create_habit_name_too_long);
    tcase_add_test(tc, test_create_habit_description_too_long);

    /* destroy_habit */
    tcase_add_test(tc, test_destroy_habit_null_does_not_crash);
    tcase_add_test(tc, test_destroy_habit_frees_records);

    /* checkIn */
    tcase_add_test(tc, test_checkin_null_habit);
    tcase_add_test(tc, test_checkin_null_date);
    tcase_add_test(tc, test_checkin_invalid_date_month);
    tcase_add_test(tc, test_checkin_invalid_date_day);
    tcase_add_test(tc, test_checkin_invalid_date_negative_year);
    tcase_add_test(tc, test_checkin_new_record);
    tcase_add_test(tc, test_checkin_same_date_twice_does_not_duplicate);
    tcase_add_test(tc, test_checkin_multiple_distinct_dates);

    /* habit_get_status */
    tcase_add_test(tc, test_get_habit_status_null_habit);
    tcase_add_test(tc, test_get_habit_status_archived);
    tcase_add_test(tc, test_get_habit_status_active_with_recent_checkin);
    tcase_add_test(tc, test_get_habit_status_inactive_with_old_checkin);
    tcase_add_test(tc, test_get_habit_status_no_checkins_is_inactive);

    /* current_streak */
    tcase_add_test(tc, test_current_streak_null_habit);
    tcase_add_test(tc, test_current_streak_no_records);
    tcase_add_test(tc, test_current_streak_single_completed_day);
    tcase_add_test(tc, test_current_streak_consecutive_days);
    tcase_add_test(tc, test_current_streak_breaks_on_gap);
    tcase_add_test(tc, test_current_streak_zero_when_most_recent_incomplete);

    /* is_active */
    tcase_add_test(tc, test_is_active_null);
    tcase_add_test(tc, test_is_active_true_for_recent_checkin);
    tcase_add_test(tc, test_is_active_false_for_old_checkin);
    tcase_add_test(tc, test_is_active_false_when_archived);

    /* calculate_best_streak */
    tcase_add_test(tc, test_best_streak_null_habit);
    tcase_add_test(tc, test_best_streak_no_records);
    tcase_add_test(tc, test_best_streak_single_day);
    tcase_add_test(tc, test_best_streak_finds_longest_past_streak);
    tcase_add_test(tc, test_best_streak_all_incomplete_is_zero);
    tcase_add_test(tc, test_best_streak_resets_after_gap_and_incomplete_day);

    suite_add_tcase(s, tc);
    return s;
}