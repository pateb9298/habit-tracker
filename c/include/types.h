#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define HABIT_NAME_LENGTH 100
#define HABIT_DESC_LENGTH 500
#define HABIT_CATEGORY_LENGTH 50

/* ============================================================
 * Status Codes
 *
 * Unified status codes returned by all subsystems.
 * Each module uses a subset of these values.
 * ============================================================ */

typedef enum {
    /* Success */
    OK = 0,

    /* Generic errors */
    INVALID_ARGUMENT,
    NULL_POINTER,
    NO_MEMORY,
    BOUNDS_EXCEEDED,
    INTERNAL_ERROR,

    /* Habit-specific errors */
    HABIT_NOT_FOUND,
    HABIT_ALREADY_EXISTS,
    HABIT_INVALID_HABIT_ID,
    HABIT_INVALID_HABIT_NAME,
    HABIT_INVALID_HABIT_DESC,
    HABIT_INVALID_STATUS,
    

    /* HabitTracker-specific errors */
    HT_NO_SUCH_HABIT,

    
    /* DataLoader-specific errors */
    DL_ERR_CONFIG,
    DL_ERR_DATAGEN,
    DL_ERR_NO_MEMORY,

    /* Tree specific errors */
    T_ERROR_TREE_EMPTY,
    T_ERROR_TREE_NULL,
    T_ERROR_INVALID_NODE,
} Status;


/* ============================================================
 * HabitGenStatus
 *
 * Status codes specific to the habit generator module.
 * ============================================================ */
typedef enum {
    HABIT_GEN_OK = 0,
    HABIT_GEN_ERR_INVALID_ARG = 1,
    HABIT_GEN_ERR_ALLOC = 2,
    HABIT_GEN_ERR_IO = 3
} HabitGenStatus;


/* ============================================================
 * StreakStatus
 *
 * Represents the existance of.
 * ============================================================ */

typedef enum {
    STREAK_NOT_EXISTING = 0,
    STREAK_EXISTS
} StreakStatus;

/* ============================================================
 * DateStatus
 *
 * Represents the validity of a date.
 * ============================================================ */
typedef enum {
    DATE_OK = 0,
    DATE_INVALID
} DateStatus;

/* ============================================================
 * Date
 *
 * Represents a calendar date used throughout the habit tracker.
 *
 * Dates are used to identify habit completion records and
 * calculate statistics such as streaks and completion rates.
 * ============================================================ */
typedef struct {

    int year;
    int month;
    int day;

} Date;

/* ============================================================
 * CompletionRecord
 *
 * Represents the completion status of a habit on a specific day.
 *
 * Records are stored as a linked list within a Habit object.
 * Each date should appear at most once per habit.
 * ============================================================ */
typedef struct CompletionRecord {

    Date date;
    bool completed;

    struct CompletionRecord* next;

} CompletionRecord;

/*CompletionRecord specific errors*/
typedef enum {
    CR_ERR_OK = 0,
    CR_ERR_INVALID_DATE,
    CR_ERR_INVALID_HABIT,
} CompletionRecordStatus;

/* ============================================================
 * HabitStatus
 *
 * Defines the current state of a habit.
 *
 * Active habits participate in tracking and reporting.
 * Archived habits are retained for historical purposes
 * but are not actively tracked.
 * ============================================================ */

typedef enum {

    HABIT_ACTIVE,
    HABIT_INACTIVE,
    HABIT_ARCHIVED,
} HabitStatus;




/* ============================================================
 * Charset
 *
 * Rendering characters used for ASCII display.
 *
 * NOTE: The 'pushable' field is for future assignments (A2/A3)
 * and is not used in A1.
 * ============================================================ */

typedef struct {
    char wall;
    char floor;
    char player;
    char treasure;
    char portal;
    char pushable; 
} Charset;


/* Forward declaration for opaque Room in public APIs */
typedef struct Room Room;

#endif /* TYPES_H */
