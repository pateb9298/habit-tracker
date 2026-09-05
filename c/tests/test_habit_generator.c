#include <check.h>

#include <stdio.h>
#include <stdlib.h>

#include "habit_generator.h"

START_TEST(test_generate_habits_count_and_ids)
{
    Habit *habits = NULL;
    size_t i = 0;

    ck_assert_int_eq(generate_habits(5, &habits, 1234), HABIT_GEN_OK);
    ck_assert_ptr_nonnull(habits);

    for (i = 0; i < 5; i++)
    {
        ck_assert_int_eq(habits[i].id, (int)(i + 1));
        ck_assert_int_gt((int)habits[i].name[0], 0);
        ck_assert_int_ge(habits[i].total_checkins, 0);

        if (habits[i].total_checkins > 0)
        {
            ck_assert_ptr_nonnull(habits[i].records);
            ck_assert_int_ge(habits[i].last_checkin_day.year, 2020);
            ck_assert_int_ge(habits[i].last_checkin_day.month, 1);
            ck_assert_int_le(habits[i].last_checkin_day.month, 12);
            ck_assert_int_ge(habits[i].last_checkin_day.day, 1);
            ck_assert_int_le(habits[i].last_checkin_day.day, 28);
        }
    }

    free_generated_habits(habits);
}
END_TEST

START_TEST(test_generate_habits_null_out_ptr)
{
    ck_assert_int_eq(generate_habits(3, NULL, 42), HABIT_GEN_ERR_INVALID_ARG);
}
END_TEST

START_TEST(test_write_json_roundtrip_smoke)
{
    const char *out_path = "habits_test.json";
    Habit *habits = NULL;
    FILE *f = NULL;

    ck_assert_int_eq(generate_habits(3, &habits, 7), HABIT_GEN_OK);
    ck_assert_int_eq(write_habits_to_json(out_path, habits, 3), HABIT_GEN_OK);

    f = fopen(out_path, "r");
    ck_assert_ptr_nonnull(f);
    if (f != NULL)
    {
        fclose(f);
    }

    free_generated_habits(habits);
}
END_TEST

START_TEST(test_write_json_invalid_arguments)
{
    Habit h = {0};

    ck_assert_int_eq(write_habits_to_json(NULL, &h, 1), HABIT_GEN_ERR_INVALID_ARG);
    ck_assert_int_eq(write_habits_to_json("habits_test.json", NULL, 1), HABIT_GEN_ERR_INVALID_ARG);
    ck_assert_int_eq(write_habits_to_json("habits_test.json", NULL, 0), HABIT_GEN_OK);
}
END_TEST

Suite *habit_generator_suite(void)
{
    Suite *s = suite_create("HabitGenerator");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_generate_habits_count_and_ids);
    tcase_add_test(tc, test_generate_habits_null_out_ptr);
    tcase_add_test(tc, test_write_json_roundtrip_smoke);
    tcase_add_test(tc, test_write_json_invalid_arguments);
    suite_add_tcase(s, tc);

    return s;
}
