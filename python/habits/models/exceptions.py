"""Exception hierarchy for Habit Tracker.

Maps C error codes to Python exceptions for proper error handling.
"""


class HabitError(Exception):
    """Base exception for all habit tracker errors."""


class InvalidArgumentError(HabitError):
    """Raised when invalid arguments are provided."""


class NullPointerError(HabitError):
    """Raised when a NULL pointer is encountered."""


class MemoryError(HabitError):
    """Raised when memory allocation fails."""


class BoundsExceededError(HabitError):
    """Raised when array bounds are exceeded."""


class InternalError(HabitError):
    """Raised when an internal error occurs."""


class HabitNotFoundError(HabitError):
    """Raised when a habit is not found."""


class HabitAlreadyExistsError(HabitError):
    """Raised when trying to create a habit that already exists."""


class InvalidHabitIdError(HabitError):
    """Raised when an invalid habit ID is provided."""


class HabitTrackerError(HabitError):
    """Raised for habit tracker-specific errors."""


STATUS_TO_EXCEPTION = {
    0: None,
    1: InvalidArgumentError,
    2: NullPointerError,
    3: MemoryError,
    4: BoundsExceededError,
    5: InternalError,
    6: HabitNotFoundError,
    7: HabitAlreadyExistsError,
    8: InvalidHabitIdError,
    9: InvalidArgumentError,
    10: InvalidArgumentError,
    11: InvalidArgumentError,
    12: HabitTrackerError,
    13: InternalError,
    14: InternalError,
    15: MemoryError,
    16: InternalError,
    17: InternalError,
    18: InternalError,
}


def check_status(status):
    """Check C function status and raise the mapped Python exception."""
    if status != 0:
        exc_class = STATUS_TO_EXCEPTION.get(status, HabitError)
        if exc_class:
            raise exc_class(f"C library error: status code {status}")
