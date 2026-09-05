"""Status enum model used by ctypes bindings and wrappers."""

from ctypes import c_int
# pylint: disable=too-few-public-methods

# A Status value is just a plain C int underneath
# Here are the named constants for readability
class Status(c_int):
    """C Status enum."""

    OK = 0
    INVALID_ARGUMENT = 1
    NULL_POINTER = 2
    NO_MEMORY = 3
    BOUNDS_EXCEEDED = 4
    INTERNAL_ERROR = 5
    HABIT_NOT_FOUND = 6
    HABIT_ALREADY_EXISTS = 7
    HABIT_INVALID_HABIT_ID = 8
    HABIT_INVALID_HABIT_NAME = 9
    HABIT_INVALID_HABIT_DESC = 10
    HABIT_INVALID_STATUS = 11
    HT_NO_SUCH_HABIT = 12
    DL_ERR_CONFIG = 13
    DL_ERR_DATAGEN = 14
    DL_ERR_NO_MEMORY = 15
    T_ERROR_TREE_EMPTY = 16
    T_ERROR_TREE_NULL = 17
    T_ERROR_INVALID_NODE = 18
