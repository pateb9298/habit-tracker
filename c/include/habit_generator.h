#ifndef HABIT_GENERATOR_H
#define HABIT_GENERATOR_H

#include <stddef.h>
#include "types.h"
#include "habit.h"


HabitGenStatus generate_habits(size_t habit_count, Habit **out_habits, unsigned int seed);
HabitGenStatus write_habits_to_json(const char *output_path, const Habit *habits, size_t habit_count);
HabitGenStatus generate_habits_json(const char *output_path, size_t habit_count, unsigned int seed);
HabitGenStatus generate_habits_json_from_config(const char *config_path, const char *output_path);

/* Datagen session API used by the loader. */
HabitGenStatus start_datagen(const char *config_path);
void stop_datagen(void);
void reset_datagen_iterator(void);
const Habit *next_generated_habit(void);
const Habit *first_generated_habit(void);
size_t generated_habit_count(void);

void free_generated_habits(Habit *habits);

#endif
