"""Habit status enum model used by ctypes bindings and wrappers."""

from ctypes import c_int
# pylint: disable=too-few-public-methods

# A Status value is just a plain C int underneath
# Here are the named constatnts for readability
class HabitStatus(c_int):
    """C HabitStatus enum."""

    HABIT_ACTIVE = 0
    HABIT_INACTIVE = 1
    HABIT_ARCHIVED = 2
