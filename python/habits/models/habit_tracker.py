"""Model wrapper around the native C HabitTracker pointer."""

import ctypes
from ctypes import POINTER, byref, c_char_p, c_int

from .. import bindings
from ..exceptions import check_status

Date = bindings.Date
# pylint: disable=protected-access

def _to_date(value):
    """Accept a Date instance or a (year, month, day) tuple/list."""
    # If it is already a Date instance, return it as is
    if isinstance(value, Date):
        return value
    # Unpack if it is a tuple or list
    year, month, day = value
    # Create a new Date instance and return it
    return Date(year=year, month=month, day=day)

class HabitTrackerWrapper:
    """Safe wrapper for a C HabitTracker instance."""

    # runs automatically whever a new instance of this class is created
    def __init__(self, tracker_ptr):
        # tracker_ptr is whatever was passed it 
        # stores it as an attribute under the name _ptr
        self._ptr = tracker_ptr

    def destroy(self):
        """Destroy the tracker and free all associated memory."""
        if self._ptr:
            # call to C code from Python into the .so library
            # Execution jumps into the real C funciton
            bindings._habit_tracker_destroy(self._ptr)
            #once control is back to python, it does another cleanup
            self._ptr = None

    def add_habit(self, name, description, created_date):
        '''Add habit, return new habit id'''
        # creating an empty output buffer to hand C by reference.
        # it puts the result into this buffer, which can be read after the C function returns
        habit_id = c_int()
        status = bindings._add_habit(
            self._ptr,
            # encode the name and description as UTF-8 strings for C function
            name.encode('utf-8'),
            description.encode('utf-8'),
            # force date to use Date struct if tuple
            _to_date(created_date),
            # gives the address of habit_id to the C function, 
            # so that it can write the new habit id into it
            byref(habit_id)
        )
        check_status(status)
        # return because C parameter has _out 
        return habit_id.value

    def check_in_habit(self, habit_id, date):
        '''check in for given date and habit id'''
        status = bindings._check_in_habit(self._ptr, habit_id, byref(_to_date(date)))
        check_status(status)

    def archive_habit(self, habit_id):
        '''archive habit with given id'''
        status = bindings._archive_habit(self._ptr, habit_id)
        check_status(status)

    def get_habit_status(self, habit_id):
        '''get habit status'''
        # int becaues HabitStatus is an enum
        status_out = c_int()
        status = bindings._get_habit_status(
            self._ptr,
            habit_id,
            byref(status_out)
        )
        check_status(status)
        return status_out.value

    def get_habit_streaks(self, habit_id):
        '''return a tuple with current streak and best streak'''
        # create 2 c_int instances to hold curr and best streak values
        current_streak = c_int()
        best_streak = c_int()
        status = bindings._get_habit_streaks(
            self._ptr,
            habit_id,
            byref(current_streak),
            byref(best_streak)
        )
        check_status(status)
        return (current_streak.value, best_streak.value)

    def get_habit_checkin_count(self, habit_id):
        '''return checking count for given habit id'''
        checkin_count_out = c_int()
        status = bindings._get_habit_checkin_count(
            self._ptr,
            habit_id,
            byref(checkin_count_out)
        )
        check_status(status)
        return checkin_count_out.value

    def get_habit_last_checkin_day(self, habit_id):
        '''return last checkin date for the habit'''
        last_checkin_day_out = Date()
        status = bindings._get_habit_last_checkin_day(
            self._ptr,
            habit_id,
            byref(last_checkin_day_out)
        )
        check_status(status)
        return last_checkin_day_out

    def get_habit_name(self, habit_id):
        '''return habit name (hint decode utf-8)'''
        # allocate 100 byte buffer for C to fill in wiith habit name
        name_out = ctypes.create_string_buffer(100)
        status = bindings._get_habit_name(
            self._ptr,
            habit_id,
            name_out,
            100
        )
        check_status(status)
        #.decode('utf-8') converts bytes back into a normal Python string
        return name_out.value.decode('utf-8')

    def get_habit_description(self, habit_id):
        '''return habit description (hint decode utf-8)'''
        # allocate 500 byte buffer for C to fill in with habit description
        desc_out = ctypes.create_string_buffer(500)
        status = bindings._get_habit_description(
            self._ptr,
            habit_id,
            desc_out,
            500
        )
        check_status(status)
        #.decode('utf-8') converts bytes back into a normal Python string
        return desc_out.value.decode('utf-8')

    def reset(self):
        '''reset habit tracker'''
        status = bindings._habit_tracker_reset(self._ptr)
        check_status(status)

    def render_all_habits(self):
        '''render all habits, return string'''
        # holds the pointer to the rendered string returned
        rendered_out = c_char_p()
        status = bindings._habit_tracker_render_all_habits(self._ptr, byref(rendered_out))
        check_status(status)
        result = rendered_out.value.decode('utf-8') if rendered_out.value else "" # pylint: disable=no-member,using-constant-test
        bindings._habit_tracker_free_string(rendered_out)
        return result

    def render_habit(self, habit_id):
        '''render habit, return string'''
        # holds the pointer to the rendered string returned
        rendered_out = c_char_p()
        status = bindings._habit_tracker_render_habit(self._ptr, habit_id, byref(rendered_out))
        check_status(status)
        result = rendered_out.value.decode('utf-8') if rendered_out.value else "" # pylint: disable=no-member,using-constant-test

        bindings._habit_tracker_free_string(rendered_out)
        return result

    def get_habit_ids(self):
        '''retrieve all habit ids as a list'''
        # because C function returns a pointer to an array of integers,
        # need to create a POINTER(c_int) instance to hold the pointer to the array
        ids_out = POINTER(c_int)()
        count_out = c_int()
        status = bindings._habit_tracker_get_habit_ids(self._ptr, byref(ids_out), byref(count_out))
        check_status(status)
        ids_list = [ids_out[i] for i in range(count_out.value)]
        bindings._habit_tracker_free_array(ids_out)
        return ids_list

    def get_habit_category(self, habit_id):
        """Return the category string for the given habit id."""
        category_out = ctypes.create_string_buffer(50)
        status = bindings._get_habit_category(
            self._ptr,
            habit_id,
            category_out,
            50
        )
        check_status(status)
        return category_out.value.decode('utf-8')

    def set_habit_category(self, habit_id, category):
        """Set the category for the given habit id."""
        status = bindings._set_habit_category(
            self._ptr,
            habit_id,
            category.encode('utf-8')
        )
        check_status(status)

    def search_habits_by_name(self, query):
        """Return list of habit ids matching the query string."""
        ids_out = POINTER(c_int)()
        count_out = c_int()
        status = bindings._habit_search_by_name(
            self._ptr,
            query.encode('utf-8'),
            byref(ids_out),
            byref(count_out)
        )
        check_status(status)
        ids_list = [ids_out[i] for i in range(count_out.value)]
        bindings._habit_tracker_free_array(ids_out)
        return ids_list

    def get_completed_today(self):
        """Return the number of habits completed today."""
        count_out = c_int()
        status = bindings._habit_tracker_completed_today(
            self._ptr,
            byref(count_out)
        )
        check_status(status)
        return count_out.value

    def edit_habit(self, habit_id, new_name, new_desc):
        """Update the name and description of a habit."""
        status = bindings._edit_habit(
            self._ptr,
            habit_id,
            new_name.encode('utf-8'),
            new_desc.encode('utf-8')
        )
        check_status(status)