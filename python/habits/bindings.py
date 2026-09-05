"""
ctypes bindings for the C habit tracker library.

Provides low-level access to libhabittracker.so functions and structures.
"""
# pylint: disable=too-few-public-methods
import ctypes
import os
from ctypes import c_int, c_char_p, c_void_p, POINTER

from .models.date import Date
from .models.habit_status import HabitStatus
from .models.status import Status

# Find and load the shared library
def _find_library():
    """Locate libhabittracker.so in the filesystem."""
    # Check multiple possible locations
    possible_paths = [
        os.path.join(os.path.dirname(__file__), '../dist/libhabittracker.so'),
        os.path.join(os.path.dirname(__file__), '../../c/lib/libhabittracker.so'),
        os.path.join(os.path.dirname(__file__), '../../lib/libhabittracker.so'),
        os.environ.get('HABITTRACKER_LIB', ''),
    ]
    for path in possible_paths:
        if path and os.path.exists(path):
            return os.path.abspath(path)
    # Try loading from system library path
    try:
        return ctypes.util.find_library('habittracker')
    except OSError:
        return None
    raise RuntimeError("Could not find libhabittracker.so")

# Opaque type for HabitTracker
class HabitTracker(ctypes.c_void_p):
    """Opaque HabitTracker pointer."""

# ============================================================
# Load the shared library and define C functions
# ============================================================

try:
    lib_path = _find_library()
    lib = ctypes.CDLL(lib_path)
except Exception as e:
    raise ImportError(f"Failed to load libhabittracker.so: {e}") from e



# ============================================================
# C Function Definitions
# ============================================================

# Python's ctypes module can't automatically know a C function's signature. so requires argtypes/restypes 

# Creation & Destruction
_habit_tracker_create = lib.habit_tracker_create

# c_char_p represents C string (char*)
# Whne the function is called,  config_file.encode('utf-8') will be
# passed to the C function as a C string.

# POINTER means a pointer to a C type. 
# Here, for HabitTracker**, (a pointer to a pointer)
# "output parameter" (because C wants to write the new tracker's address into a variable)
_habit_tracker_create.argtypes = [c_char_p, POINTER(c_void_p)]

# Returns a int (Status enum) --> so it is just an int
_habit_tracker_create.restype = c_int

_habit_tracker_destroy = lib.habit_tracker_destroy
_habit_tracker_destroy.argtypes = [c_void_p]
_habit_tracker_destroy.restype = None

# Habit Operations
_add_habit = lib.add_habit
_add_habit.argtypes = [c_void_p, c_char_p, c_char_p, Date, POINTER(c_int)]
_add_habit.restype = c_int

_check_in_habit = lib.check_in_habit
_check_in_habit.argtypes = [c_void_p, c_int, POINTER(Date)]
_check_in_habit.restype = c_int

_archive_habit = lib.archive_habit
_archive_habit.argtypes = [c_void_p, c_int]
_archive_habit.restype = c_int

# Habit Queries
_get_habit_status = lib.get_habit_status
_get_habit_status.argtypes = [c_void_p, c_int, POINTER(c_int)]
_get_habit_status.restype = c_int

_get_habit_streaks = lib.get_habit_streaks
_get_habit_streaks.argtypes = [c_void_p, c_int, POINTER(c_int), POINTER(c_int)]
_get_habit_streaks.restype = c_int

_get_habit_checkin_count = lib.get_habit_checkin_count
_get_habit_checkin_count.argtypes = [c_void_p, c_int, POINTER(c_int)]
_get_habit_checkin_count.restype = c_int

_get_habit_last_checkin_day = lib.get_habit_last_checkin_day
_get_habit_last_checkin_day.argtypes = [c_void_p, c_int, POINTER(Date)]
_get_habit_last_checkin_day.restype = c_int

_get_habit_name = lib.get_habit_name
_get_habit_name.argtypes = [c_void_p, c_int, c_char_p, ctypes.c_ulong]
_get_habit_name.restype = c_int

_get_habit_description = lib.get_habit_description
_get_habit_description.argtypes = [c_void_p, c_int, c_char_p, ctypes.c_ulong]
_get_habit_description.restype = c_int

# Reset
_habit_tracker_reset = lib.habit_tracker_reset
_habit_tracker_reset.argtypes = [c_void_p]
_habit_tracker_reset.restype = c_int

# Rendering
_habit_tracker_render_all_habits = lib.habit_tracker_render_all_habits
_habit_tracker_render_all_habits.argtypes = [c_void_p, POINTER(c_char_p)]
_habit_tracker_render_all_habits.restype = c_int

_habit_tracker_render_habit = lib.habit_tracker_render_habit
_habit_tracker_render_habit.argtypes = [c_void_p, c_int, POINTER(c_char_p)]
_habit_tracker_render_habit.restype = c_int

# Habit IDs
_habit_tracker_get_habit_ids = lib.habit_tracker_get_habit_ids
_habit_tracker_get_habit_ids.argtypes = [c_void_p, POINTER(POINTER(c_int)), POINTER(c_int)]
_habit_tracker_get_habit_ids.restype = c_int

# Memory Management
_habit_tracker_free_string = lib.habit_tracker_free_string
_habit_tracker_free_string.argtypes = [c_char_p]
_habit_tracker_free_string.restype = c_int

_habit_tracker_free_array = lib.habit_tracker_free_array
# takes void ptr because it can be an array of any type (int, char*, etc.)
_habit_tracker_free_array.argtypes = [c_void_p]
_habit_tracker_free_array.restype = c_int

# Category functions
_get_habit_category = lib.get_habit_category
_get_habit_category.argtypes = [c_void_p, c_int, c_char_p, ctypes.c_ulong]
_get_habit_category.restype = c_int

_set_habit_category = lib.set_habit_category
_set_habit_category.argtypes = [c_void_p, c_int, c_char_p]
_set_habit_category.restype = c_int

# Search function
_habit_search_by_name = lib.habit_search_by_name
_habit_search_by_name.argtypes = [c_void_p, c_char_p, POINTER(POINTER(c_int)), POINTER(c_int)]
_habit_search_by_name.restype = c_int

# Completed today function
_habit_tracker_completed_today = lib.habit_tracker_completed_today
_habit_tracker_completed_today.argtypes = [c_void_p, POINTER(c_int)]
_habit_tracker_completed_today.restype = c_int

# Edit habit function
_edit_habit = lib.edit_habit
_edit_habit.argtypes = [c_void_p, c_int, c_char_p, c_char_p]
_edit_habit.restype = c_int

# ============================================================
# Export public API
# ============================================================

# Decides what gets imported when someone does a wildcard import:
# ex: from habits.bindings import *
__all__ = [
    'Date',
    'HabitStatus',
    'Status',
    'HabitTracker',
    'lib',
]
