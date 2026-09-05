"""
HabitTracker class providing a Pythonic interface to the habit tracking system.

This module provides the main user-facing API for managing habits and tracking streaks.
"""

from .wrappers import create_tracker, Date, HabitStatus
from .exceptions import HabitError


class HabitTracker:
    """High-level Python interface to the habit tracking system."""
    STATUS_NAMES = {
        HabitStatus.HABIT_ACTIVE: "active",
        HabitStatus.HABIT_INACTIVE: "inactive",
        HabitStatus.HABIT_ARCHIVED: "archived",
    }
    def __init__(self, config_file):
        """
        Initialize the HabitTracker from a configuration file.

        Args:
            config_file: Path to the configuration file (string)

        Raises:
            HabitError: If initialization fails
        """
        self._wrapper = create_tracker(config_file)

    def close(self):
        """Close the tracker and free all resources."""
        # prevents crashing if close() gets called twice??
        if getattr(self, "_wrapper", None) is not None:
            self._wrapper.destroy()
            self._wrapper = None

    def __del__(self):
        """Ensure tracker is cleaned up on deletion."""
        try:
            self.close()
        except Exception: # pylint: disable=broad-exception-caught
            pass

    def __enter__(self):
        """Context manager entry."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.close()

    # Helper method to convert a date input to a Date object???
    def _to_date(self, value):
        if isinstance(value, Date):
            return value
        year, month, day = value
        return Date(year=year, month=month, day=day)

    def add_habit(self, name, description, created_date):
        """
        Add a new habit to the tracker.

        Args:
            name: Habit name (string)
            description: Habit description (string)
            created_date: Date object or tuple (year, month, day)

        Returns:
            int: Habit ID

        Raises:
            HabitError: If the operation fails
        """
        return self._wrapper.add_habit(name, description, self._to_date(created_date))

    def check_in_habit(self, habit_id, date):
        """
        Check in for a habit on a specific date.

        Args:
            habit_id: ID of the habit
            date: Date object or tuple (year, month, day)

        Raises:
            HabitError: If the operation fails
        """
        self._wrapper.check_in_habit(habit_id, self._to_date(date))

    def archive_habit(self, habit_id):
        """
        Archive a habit.

        Args:
            habit_id: ID of the habit

        Raises:
            HabitError: If the operation fails
        """
        self._wrapper.archive_habit(habit_id)

    def get_habit_status(self, habit_id):
        """
        Get the status of a habit.

        Args:
            habit_id: ID of the habit

        Returns:
            str: Status ("active", "inactive", or "archived")

        Raises:
            HabitError: If the operation fails
        """
        status_val = self._wrapper.get_habit_status(habit_id)
        return self.STATUS_NAMES.get(status_val)

    def get_habit_streaks(self, habit_id):
        """
        Get current and best streaks for a habit.

        Args:
            habit_id: ID of the habit

        Returns:
            dict: {"current": int, "best": int}

        Raises:
            HabitError: If the operation fails
        """
        current, best = self._wrapper.get_habit_streaks(habit_id)
        return {"current": current, "best": best}

    def get_habit_checkin_count(self, habit_id):
        """
        Get the number of check-ins for a habit.

        Args:
            habit_id: ID of the habit

        Returns:
            int: Number of check-ins

        Raises:
            HabitError: If the operation fails
        """
        return self._wrapper.get_habit_checkin_count(habit_id)

    def get_habit_last_checkin_day(self, habit_id):
        """
        Get the date of the last check-in for a habit.

        Args:
            habit_id: ID of the habit

        Returns:
            dict: {"year": int, "month": int, "day": int}

        Raises:
            HabitError: If the operation fails
        """
        date = self._wrapper.get_habit_last_checkin_day(habit_id)
        return {"year": date.year, "month": date.month, "day": date.day}

    def get_habit_info(self, habit_id):
        """
        Get all available information about a habit.

        Args:
            habit_id: ID of the habit

        Returns:
            dict: Habit information

        Raises:
            HabitError: If the operation fails
        """
        current, best = self._wrapper.get_habit_streaks(habit_id)
        last_checkin = self._wrapper.get_habit_last_checkin_day(habit_id)
        return {
            "id": habit_id,
            "name": self._wrapper.get_habit_name(habit_id),
            "description": self._wrapper.get_habit_description(habit_id),
            "status": self.get_habit_status(habit_id),
            "category": self._wrapper.get_habit_category(habit_id), 
            "current_streak": current,
            "best_streak": best,
            "total_checkins": self._wrapper.get_habit_checkin_count(habit_id),
            "last_checkin": {
                "year": last_checkin.year,
                "month": last_checkin.month,
                "day": last_checkin.day,
            },
        }

    def get_all_habits(self):
        """
        Get information about all habits.

        Returns:
            list: List of habit info dictionaries

        Raises:
            HabitError: If the operation fails
        """
        return [self.get_habit_info(habit_id) for habit_id in self._wrapper.get_habit_ids()]


    def reset(self):
        """Reset the tracker to its initial state."""
        self._wrapper.reset()

    def render_all_habits(self):
        """
        Get a formatted string representation of all habits.

        Returns:
            str: Formatted habit information

        Raises:
            HabitError: If the operation fails
        """
        return self._wrapper.render_all_habits()

    def render_habit(self, habit_id):
        """
        Get a formatted string representation of a specific habit.

        Args:
            habit_id: ID of the habit

        Returns:
            str: Formatted habit information

        Raises:
            HabitError: If the operation fails
        """
        return self._wrapper.render_habit(habit_id)

    def get_habit_ids(self):
        """
        Get all habit IDs in the tracker.

        Returns:
            list: List of habit IDs

        Raises:
            HabitError: If the operation fails
        """
        return self._wrapper.get_habit_ids()
    
    def get_habit_category(self, habit_id):
        """Get the category of a habit.
        
        Args:
            habit_id: ID of the habit
        
        Returns:
            str: Category name
        
        Raises:
            HabitError: If the operation fails
        """
        return self._wrapper.get_habit_category(habit_id)

    def set_habit_category(self, habit_id, category):
        """Set the category of a habit.
        
        Args:
            habit_id: ID of the habit
            category: Category name string
        
        Raises:
            HabitError: If the operation fails
        """
        self._wrapper.set_habit_category(habit_id, category)

    def search_habits_by_name(self, query):
        """Search habits by partial name match.
        
        Args:
            query: partial name string to search for
        
        Returns:
            list: list of habit info dicts for matching habits
        """
        ids = self._wrapper.search_habits_by_name(query)
        return [self.get_habit_info(habit_id) for habit_id in ids]

    def get_completed_today(self):
        """Get the number of habits completed today.
        
        Returns:
            int: number of habits checked in today
        """
        return self._wrapper.get_completed_today()

    def edit_habit(self, habit_id, new_name, new_desc):
        """Edit the name and description of a habit.
        
        Args:
            habit_id: ID of the habit
            new_name: new name string
            new_desc: new description string
        
        Raises:
            HabitError: If the operation fails
        """
        self._wrapper.edit_habit(habit_id, new_name, new_desc)

__all__ = [
    'HabitTracker',
    'HabitError',
]
