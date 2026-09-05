#include "habit_generator.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//holds settings parsed from the .ini config file
typedef struct {
    size_t num_habits;
    unsigned int seed;
    int checkins_no;
    int archived_chance;
} HabitGenConfig;

//global array of generated habits 
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static Habit *g_generated_habits = NULL;

// how many habits are in g_generated_habits
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static size_t g_generated_count = 0;

//current position in the iteration 
// continues with next_generated_habit() call
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static size_t g_generated_index = 0;

// retunrs a random integer between min_value and max_value
static int random_range(int min_value, int max_value) {
    // NOLINTNEXTLINE(cert-msc30-c,cert-msc50-cpp)
    return min_value + (rand() % (max_value - min_value + 1));
}

//clamps value so that it stays between min and max value
static int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

// generates a random habit name by picking a random prefix and action
// NOLINTNEXTLINE(readability-identifier-naming)
static void generateHabitName(char *buffer, size_t buffer_size, int habit_id) {
    static const char *prefixes[] = {
        "Morning", "Daily", "Focus", "Healthy", "Weekly", "Quick", "Calm", "Strong"
    };
    static const char *actions[] = {
        "Walk", "Read", "Stretch", "Code", "Journal", "Meditate", "Hydrate", "Practice"
    };

    size_t prefix_count = sizeof(prefixes) / sizeof(prefixes[0]);
    size_t action_count = sizeof(actions) / sizeof(actions[0]);

    // NOLINTNEXTLINE(cert-msc30-c,cert-msc50-cpp)
    const char *prefix = prefixes[(size_t)rand() % prefix_count];
    
    // NOLINTNEXTLINE(cert-msc30-c,cert-msc50-cpp)
    const char *action = actions[(size_t)rand() % action_count];

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)snprintf(buffer, buffer_size, "%s %s %d", prefix, action, habit_id);
}

//strips leading and trailing whitespace from a string in place
static char *trim(char *s) {
    char *end = NULL;

    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }

    if (*s == '\0') {
        return s;
    }

    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return s;
}

// used by load_config to parse numeric values from the .ini file
static int parse_unsigned_long(const char *text, unsigned long *out) {
    char *end = NULL;
    unsigned long value = 0;

    if (text == NULL || out == NULL || *text == '\0') {
        return 0;
    }

    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }

    *out = value;
    return 1;
}

// reads the .ini config file line by line, scripts comments and whitespace
// and stores the values into the HabitGenConfig struct
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static HabitGenStatus load_config(const char *config_path, HabitGenConfig *cfg) {
    FILE *f = NULL;
    char line[512];
    int has_num_habits = 0;

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    if (config_path == NULL || cfg == NULL) {
        return HABIT_GEN_ERR_INVALID_ARG;
    }

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    f = fopen(config_path, "r");
    if (f == NULL) {
        return HABIT_GEN_ERR_IO;
    }

    cfg->num_habits = 0;
    cfg->seed = 0;
    cfg->checkins_no = -1;
    cfg->archived_chance = 50;

    while (fgets(line, sizeof(line), f) != NULL) {
        char *content = trim(line);
        char *eq = NULL;

        if (*content == '\0' || *content == '#' || *content == ';' || *content == '[') {
            continue;
        }

        eq = strchr(content, '=');
        if (eq == NULL) {
            continue;
        }

        *eq = '\0';
        {
            const char *key = trim(content);
            const char *value = trim(eq + 1);
            unsigned long parsed = 0;
 
            if (strcmp(key, "num_habits") == 0 || strcmp(key, "habit_count") == 0) {
                if (!parse_unsigned_long(value, &parsed)) {
                    (void)fclose(f);
                    return HABIT_GEN_ERR_INVALID_ARG;
                }
                cfg->num_habits = (size_t)parsed;
                has_num_habits = 1;
            } else if (strcmp(key, "seed") == 0) {
                if (!parse_unsigned_long(value, &parsed)) {
                    (void)fclose(f);
                    return HABIT_GEN_ERR_INVALID_ARG;
                }
                cfg->seed = (unsigned int)parsed;
            } else if (strcmp(key, "checkins_no") == 0) {
                if (!parse_unsigned_long(value, &parsed)) {
                    (void)fclose(f);
                    return HABIT_GEN_ERR_INVALID_ARG;
                }
                cfg->checkins_no = (int)parsed;
            } else if (strcmp(key, "archived_chance") == 0) {
                if (!parse_unsigned_long(value, &parsed)) {
                    (void)fclose(f);
                    return HABIT_GEN_ERR_INVALID_ARG;
                }
                cfg->archived_chance = (int)parsed;
            }
        }
    }

    (void)fclose(f);

    if (!has_num_habits) {
        return HABIT_GEN_ERR_INVALID_ARG;
    }

    return HABIT_GEN_OK;
}

//takes a starting date and adds offset_days to it
static Date calculate_date_offset(const Date *start, int offset_days) {
    Date result = *start;
    result.day += offset_days;

    while (result.day > 28) {
        result.day -= 28;
        result.month++;
        if (result.month > 12) {
            result.month = 1;
            result.year++;
        }
    }

    return result;
}

//fills a single Habit struct with randomly generated data
static void fill_habit(Habit *habit, int id, int fixed_checkins, int archived_chance) {
    int total = fixed_checkins >= 0 ? fixed_checkins : random_range(0, 365);
    int i = 0;
    int year = 2020 + random_range(0, 6);
    int month = random_range(1, 12);
    int day = random_range(1, 28);
    Date created;

    habit->id = id;
    generateHabitName(habit->name, sizeof(habit->name), id);
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    (void)snprintf(habit->description, sizeof(habit->description), "Generated habit %d", id);

    habit->total_checkins = total;
    habit->records = NULL;

    created.year = year;
    created.month = month;
    created.day = day;
    habit->created_date = created;
    habit->last_checkin_day = created;

    for (i = 0; i < total; i++) {
        CompletionRecord *record = (CompletionRecord *)malloc(sizeof(CompletionRecord));
        if (record == NULL) {
            break;
        }
        record->date.year = year;
        record->date.month = month;
        record->date.day = day;
        record->date = calculate_date_offset(&record->date, i);
        // NOLINTNEXTLINE(cert-msc30-c,cert-msc50-cpp)
        record->completed = (rand() % 2) == 0;
        record->next = habit->records;
        habit->records = record;
        habit->last_checkin_day = record->date;
    }

    habit->total_checkins = i;
    // NOLINTNEXTLINE(cert-msc30-c,cert-msc50-cpp)
    habit->is_archived = (rand() % 100) < clamp_int(archived_chance, 0, 100);
}

// generates an array of habit_count habits in memory using a random seed
HabitGenStatus generate_habits(size_t habit_count, Habit **out_habits, unsigned int seed) {
    size_t i = 0;
    Habit *habits = NULL;

    if (out_habits == NULL) {
        return HABIT_GEN_ERR_INVALID_ARG;
    }

    *out_habits = NULL;

    if (habit_count == 0) {
        return HABIT_GEN_OK;
    }

    if (seed == 0) {
        seed = (unsigned int)time(NULL);
    }
    srand(seed);

    habits = (Habit *)calloc(habit_count + 1, sizeof(Habit));
    if (habits == NULL) {
        return HABIT_GEN_ERR_ALLOC;
    }

    for (i = 0; i < habit_count; i++) {
        fill_habit(&habits[i], (int)(i + 1), -1, 50);
    }

    *out_habits = habits;
    return HABIT_GEN_OK;
}

// writes an array of habits to a JSON file on disk
// this is the file that loader_load_habits reads later
HabitGenStatus write_habits_to_json(const char *output_path, const Habit *habits, size_t habit_count) {
    size_t i = 0;
    FILE *file = NULL;

    if (output_path == NULL) {
        return HABIT_GEN_ERR_INVALID_ARG;
    }
    if (habit_count > 0 && habits == NULL) {
        return HABIT_GEN_ERR_INVALID_ARG;
    }

    file = fopen(output_path, "w");
    if (file == NULL) {
        return HABIT_GEN_ERR_IO;
    }

    (void)fprintf(file, "{\n");
    (void)fprintf(file, "  \"habit_count\": %zu,\n", habit_count);
    (void)fprintf(file, "  \"habits\": [\n");

    for (i = 0; i < habit_count; i++) {
        const Habit *h = &habits[i];
        const CompletionRecord *record = h->records;
        (void)fprintf(file, "    {\n");
        (void)fprintf(file, "      \"id\": %d,\n", h->id);
        (void)fprintf(file, "      \"name\": \"%s\",\n", h->name);
        (void)fprintf(file, "      \"description\": \"%s\",\n", h->description);
        (void)fprintf(file, "      \"total_checkins\": %d,\n", h->total_checkins);
        (void)fprintf(file, "      \"is_archived\": %d,\n", h->is_archived ? 1 : 0);
        (void)fprintf(file, "      \"created_date\": \"%04d-%02d-%02d\",\n", h->created_date.year, h->created_date.month, h->created_date.day);
        (void)fprintf(file, "      \"last_checkin_day\": \"%04d-%02d-%02d\",\n", h->last_checkin_day.year, h->last_checkin_day.month, h->last_checkin_day.day);
        (void)fprintf(file, "      \"checkins\": [\n");
        while (record) {
            (void)fprintf(file, "        {\n");
            (void)fprintf(file, "          \"date\": \"%04d-%02d-%02d\",\n", record->date.year, record->date.month, record->date.day);
            (void)fprintf(file, "          \"completed\": %d\n", record->completed ? 1 : 0);
            (void)fprintf(file, "        }%s\n", record->next ? "," : "");
            record = record->next;
        }
        (void)fprintf(file, "      ]\n");
        if (i + 1 < habit_count) {
            (void)fprintf(file, "    },\n");
        } else {
            (void)fprintf(file, "    }\n");
        }
    }

    (void)fprintf(file, "  ]\n");
    (void)fprintf(file, "}\n");

    if (fclose(file) != 0) {
        return HABIT_GEN_ERR_IO;
    }

    return HABIT_GEN_OK;
}

//generates habits then immiedealy writes them to JSON
HabitGenStatus generate_habits_json(const char *output_path, size_t habit_count, unsigned int seed) {
    Habit *habits = NULL;
    HabitGenStatus status = generate_habits(habit_count, &habits, seed);

    if (status != HABIT_GEN_OK) {
        return status;
    }

    status = write_habits_to_json(output_path, habits, habit_count);
    free_generated_habits(habits);
    return status;
}

//reads settings from .ini config file then generates habits 
// and writes to JSON
HabitGenStatus generate_habits_json_from_config(const char *config_path, const char *output_path) {
    HabitGenConfig cfg;
    HabitGenStatus status = HABIT_GEN_OK;
    Habit *habits = NULL;
    size_t i = 0;

    if (output_path == NULL) {
        return HABIT_GEN_ERR_INVALID_ARG;
    }

    status = load_config(config_path, &cfg);
    if (status != HABIT_GEN_OK) {
        return status;
    }

    if (cfg.seed == 0) {
        cfg.seed = (unsigned int)time(NULL);
    }
    srand(cfg.seed);

    if (cfg.num_habits == 0) {
        return write_habits_to_json(output_path, NULL, 0);
    }

    habits = (Habit *)calloc(cfg.num_habits + 1, sizeof(Habit));
    if (habits == NULL) {
        return HABIT_GEN_ERR_ALLOC;
    }

    for (i = 0; i < cfg.num_habits; i++) {
        fill_habit(&habits[i], (int)(i + 1), cfg.checkins_no, cfg.archived_chance);
    }

    status = write_habits_to_json(output_path, habits, cfg.num_habits);
    free_generated_habits(habits);
    return status;
}

//stats a datagen session:
//reads config, generates all habits into g_generated_habits,
// prepares the iterator for next_generated_habit()
HabitGenStatus start_datagen(const char *config_path) {
    HabitGenConfig cfg;
    size_t i = 0;
    HabitGenStatus status = load_config(config_path, &cfg);

    if (status != HABIT_GEN_OK) {
        return status;
    }

    stop_datagen();

    if (cfg.seed == 0) {
        cfg.seed = (unsigned int)time(NULL);
    }
    srand(cfg.seed);

    g_generated_count = cfg.num_habits;
    g_generated_index = 0;

    if (g_generated_count == 0) {
        return HABIT_GEN_OK;
    }

    g_generated_habits = (Habit *)calloc(g_generated_count + 1, sizeof(Habit));
    if (g_generated_habits == NULL) {
        g_generated_count = 0;
        return HABIT_GEN_ERR_ALLOC;
    }

    for (i = 0; i < g_generated_count; i++) {
        fill_habit(&g_generated_habits[i], (int)(i + 1), cfg.checkins_no, cfg.archived_chance);
    }

    return HABIT_GEN_OK;
}

// ends the datagen session, frees memory and resets all globals
void stop_datagen(void) {
    if (g_generated_habits != NULL) {
        free_generated_habits(g_generated_habits);
        g_generated_habits = NULL;
    }

    g_generated_count = 0;
    g_generated_index = 0;
}

// resets iterator back to the beggining of g_genearted habits
void reset_datagen_iterator(void) {
    g_generated_index = 0;
}

const Habit *next_generated_habit(void) {
    const Habit *out = NULL;

    if (g_generated_habits == NULL || g_generated_index >= g_generated_count) {
        return NULL;
    }

    out = &g_generated_habits[g_generated_index];
    g_generated_index++;
    return out;
}

const Habit *first_generated_habit(void) {
    if (g_generated_habits == NULL || g_generated_count == 0) {
        return NULL;
    }

    return &g_generated_habits[0];
}

size_t generated_habit_count(void) {
    return g_generated_count;
}

// frees an array of habits and all their completion record linked lists
void free_generated_habits(Habit *habits) {
    size_t i = 0;

    if (habits == NULL) {
        return;
    }

    while (1) {
        CompletionRecord *record = habits[i].records;
        while (record != NULL) {
            CompletionRecord *next = record->next;
            free(record);
            record = next;
        }

        if (habits[i].id == 0 &&  habits[i].records == NULL && habits[i].name[0] == '\0') {
            break;
        }
        i++;
    }

    free(habits);
}
