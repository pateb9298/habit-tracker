#include <check.h>
#include <stdlib.h>

Suite *habit_generator_suite(void);
Suite *habit_loader_suite(void);

Suite *habit_suite(void);
Suite *habit_tracker_suite(void);

int main(void) {
    Suite *suites[] = {
        habit_generator_suite(),
        habit_loader_suite(),
        habit_suite(),
        NULL
    };

    SRunner *runner = srunner_create(suites[0]);
    for (int i = 1; suites[i] != NULL; ++i) {
        srunner_add_suite(runner, suites[i]);
    }

    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
