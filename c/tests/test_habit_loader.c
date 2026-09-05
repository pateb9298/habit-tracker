#include <check.h>

#include "habit_generator.h"
#include "habit_loader.h"
#include "tree.h"

START_TEST(test_loader_grading1_count)
{
    const char *json_path = "1_habits.json";
    Tree *tree = NULL;
    int count = -1;

    ck_assert_int_eq(generate_habits_json(json_path, 3, 101), HABIT_GEN_OK);
    ck_assert_int_eq(loader_load_habits(json_path, &tree, &count), OK);
    ck_assert_ptr_nonnull(tree);
    ck_assert_int_eq(count, 3);

    destroy_tree(tree);
}
END_TEST

START_TEST(test_loader_grading2_count)
{
    const char *json_path = "2_habits.json";
    Tree *tree = NULL;
    int count = -1;

    ck_assert_int_eq(generate_habits_json(json_path, 10, 202), HABIT_GEN_OK);
    ck_assert_int_eq(loader_load_habits(json_path, &tree, &count), OK);
    ck_assert_ptr_nonnull(tree);
    ck_assert_int_eq(count, 10);

    destroy_tree(tree);
}
END_TEST

START_TEST(test_loader_grading3_count)
{
    const char *json_path = "3_habits.json";
    Tree *tree = NULL;
    int count = -1;

    ck_assert_int_eq(generate_habits_json(json_path, 50, 303), HABIT_GEN_OK);
    ck_assert_int_eq(loader_load_habits(json_path, &tree, &count), OK);
    ck_assert_ptr_nonnull(tree);
    ck_assert_int_eq(count, 50);

    destroy_tree(tree);
}
END_TEST

Suite *habit_loader_suite(void)
{
    Suite *s = suite_create("HabitLoader");
    TCase *tc = tcase_create("Core");

    tcase_set_timeout(tc, 60);

    tcase_add_test(tc, test_loader_grading1_count);
    tcase_add_test(tc, test_loader_grading2_count);
    tcase_add_test(tc, test_loader_grading3_count);

    suite_add_tcase(s, tc);
    return s;
}
