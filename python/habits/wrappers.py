"""Wrapper entrypoints for tracker creation and model imports."""
# pylint: disable=protected-access
import ctypes
from ctypes import c_int, byref, POINTER

from . import bindings
from .exceptions import check_status
from .models.habit_tracker import HabitTrackerWrapper

Date = bindings.Date
Status = bindings.Status
HabitStatus = bindings.HabitStatus
lib = bindings.lib

# habit_tracker_create
def create_tracker(config_file):
    """
    Create a new HabitTracker from a configuration file.

    Args:
        config_file: Path to the configuration file (string)

    Returns:
        HabitTrackerWrapper: Wrapped tracker instance
    """
    # Creates an empty pointer box for C to write new track'er memory address 
    tracker_ptr = ctypes.c_void_p()

    # Get the address of that box to pass to C
    ctypes.byref(tracker_ptr)

    # Call the C function
    status = bindings._habit_tracker_create(
        # Converts the Python str insto a bytes object ctypes can hand to C as a char*
        config_file.encode('utf-8'),

        # After this call, tracker_ptr has been filled in by C.
        # Holds the real tracker's memory address
        ctypes.byref(tracker_ptr)
    )
    # Helper in exceptions.py (looks up status in STATUS_TO_EXCEPTION
    # and raises the matching Python exception if it is not 0)
    check_status(status)

    # Wrap the pointer in the Python friendly wrapper class, and return it.
    return HabitTrackerWrapper(tracker_ptr)

def destroy_tracker(tracker_ptr):
    """Destroy a tracker instance and free its C resources."""
    # call to C code from Python into the .so library
    # Execution jumps into the real C funciton
    bindings._habit_tracker_destroy(tracker_ptr)

def add_habit(tracker_ptr, name, description, created_date):
    """Add a new habit to the tracker and return its generated id."""
    # creating an empty output buffer to hand C by reference.
    # it puts the result into this buffer, which can be read after the C function returns
    habit_id = c_int()
    status = bindings._add_habit(
        tracker_ptr,
        # encode the name and description as UTF-8 strings for C function
        name.encode('utf-8'),
        description.encode('utf-8'),
        created_date,
        # gives the address of habit_id to the C function, so that it can write the new habit id into it
        byref(habit_id)
    )
    check_status(status)
    # return because C parameter has _out 
    return habit_id.value 


def check_in_habit(tracker_ptr, habit_id, date):
    """Record a check-in for the given habit id and date."""
    status = bindings._check_in_habit(
        tracker_ptr,
        habit_id,
        byref(date)
    )
    check_status(status)

def archive_habit(tracker_ptr, habit_id):
    """Archive the habit with the given id."""
    status = bindings._archive_habit(
        tracker_ptr,
        habit_id
    )
    check_status(status)

def get_habit_status(tracker_ptr, habit_id):
    """Return the HabitStatus int value for the given habit id."""
    status_out = c_int()
    status = bindings._get_habit_status(
        tracker_ptr,
        habit_id,
        byref(status_out)
    )
    check_status(status)
    return status_out.value

def get_habit_streaks(tracker_ptr, habit_id, current_streak, best_streak):
    """Return a (current_streak, best_streak) tuple for the given habit id."""
    # create 2 c_int instances to hold curr and best streak values
    current_streak = c_int()
    best_streak = c_int()
    status = bindings._get_habit_streaks(
        tracker_ptr,
        habit_id,
        byref(current_streak),
        byref(best_streak)
    )
    check_status(status)
    return (current_streak.value, best_streak.value)

def get_habit_checkin_count(tracker_ptr, habit_id):
    """Return the total check-in count for the given habit id."""
    checkin_count_out = c_int()
    status = bindings._get_habit_checkin_count(
        tracker_ptr,
        habit_id,
        byref(checkin_count_out)
    )
    check_status(status)
    return checkin_count_out.value

def get_habit_last_checkin_day(tracker_ptr, habit_id):
    """Return the Date of the last check-in for the given habit id."""
    last_checkin_day_out = Date()
    status = bindings._get_habit_last_checkin_day(
        tracker_ptr,
        habit_id,
        byref(last_checkin_day_out)
    )
    check_status(status)
    return last_checkin_day_out.value

def get_habit_name(tracker_ptr, habit_id, name_len):
    """Return the decoded name string for the given habit id."""
    # allocate 100 byte buffer for C to fill in wiith habit name
    name_out = ctypes.create_string_buffer(name_len)
    status = bindings._get_habit_name(
        tracker_ptr,
        habit_id,
        name_out,
        name_len
    )
    check_status(status)
    #.decode('utf-8') converts bytes back into a normal Python string
    return name_out.value.decode('utf-8')

def get_habit_description(tracker_ptr, habit_id, description_len):
    """Return the decoded description string for the given habit id."""
    # allocate 500 byte buffer for C to fill in with habit description
    description_out = ctypes.create_string_buffer(description_len)
    status = bindings._get_habit_description(
        tracker_ptr,
        habit_id,
        description_out,
        description_len
    )
    check_status(status)
    #.decode('utf-8') converts bytes back into a normal Python string
    return description_out.value.decode('utf-8')

def habit_tracker_reset(tracker_ptr):
    """Reset the tracker to its initial, freshly-loaded state."""
    status = bindings._habit_tracker_reset(tracker_ptr)
    check_status(status)

def habit_tracker_render_all_habits(tracker_ptr):
    """Return the rendered string for all habits in the tracker."""
    # holds the pointer to the rendered string returned
    rendered_out = ctypes.c_char_p()
    status = bindings._habit_tracker_render_all_habits(
        tracker_ptr,
        byref(rendered_out)
    )
    check_status(status)
    result = rendered_out.value.decode('utf-8') if rendered_out.value else "" # pylint: disable=no-member,using-constant-test
    bindings._habit_tracker_free_string(rendered_out)
    return result

def habit_tracker_render_habit(tracker_ptr, habit_id):
    """Return the rendered string for a single habit by id."""
    # holds the pointer to the rendered string returned
    rendered_out = ctypes.c_char_p()
    status = bindings._habit_tracker_render_habit(
        tracker_ptr,
        habit_id,
        byref(rendered_out)
    )
    check_status(status)
    result = rendered_out.value.decode('utf-8') if rendered_out.value else "" # pylint: disable=no-member,using-constant-test
    bindings._habit_tracker_free_string(rendered_out)
    return result

def habit_tracker_get_habit_ids(tracker_ptr):
    """Return a list of all habit ids currently in the tracker."""
    # because C function returns a pointer to an array of integers,
    # need to create a POINTER(c_int) instance to hold the pointer to the array
    ids_out = POINTER(c_int)()
    count_out = c_int()
    status = bindings._habit_tracker_get_habit_ids(
        tracker_ptr,
        byref(ids_out),
        byref(count_out)
    )
    check_status(status)
    # After the call, ids_out is a POINTER(c_int)
    # It's the memory address where an array of ints starts
    ids_list = [ids_out[i] for i in range(count_out.value)]
    # Free the allocated memory for the habit IDs
    bindings._habit_tracker_free_array(ids_out)
    return ids_list

def habit_tracker_free_string(string_ptr):
    """Free a C-allocated string returned by the habit tracker."""
    status = bindings._habit_tracker_free_string(string_ptr)
    check_status(status)

def habit_tracker_free_array(array_ptr):
    """Free a C-allocated array returned by the habit tracker."""
    status = bindings._habit_tracker_free_array(array_ptr)
    check_status(status)

__all__ = [
    "HabitTrackerWrapper",
    "create_tracker",
    "destroy_tracker",
    "Date",
    "HabitStatus",
    "Status",
]
