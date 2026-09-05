"""
Habit Tracker Python Package

Provides ctypes bindings and Pythonic wrappers for the C habit tracker library.
"""

__version__ = "2.0.0"
__author__ = "CIS2750"

from . import bindings
from . import wrappers
from . import habit_tracker
from .exceptions import HabitError, InvalidArgumentError, MemoryError as HabitMemoryError

__all__ = [
    'bindings',
    'wrappers',
    'habit_tracker',
    'HabitError',
    'InvalidArgumentError',
    'HabitMemoryError',
]
