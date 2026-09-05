"""Model classes and exceptions for the habit tracker Python package."""

from .date import Date
from .habit_status import HabitStatus
from .status import Status
from ..exceptions import (
    HabitError,
    InvalidArgumentError,
    NullPointerError,
    MemoryError,
    BoundsExceededError,
    InternalError,
    HabitNotFoundError,
    HabitAlreadyExistsError,
    InvalidHabitIdError,
    HabitTrackerError,
    check_status,
)


def __getattr__(name):
    if name == "HabitTrackerWrapper":
        from .habit_tracker import HabitTrackerWrapper

        return HabitTrackerWrapper
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")


__all__ = [
    "Date",
    "HabitStatus",
    "Status",
    "HabitTrackerWrapper",
    "HabitError",
    "InvalidArgumentError",
    "NullPointerError",
    "MemoryError",
    "BoundsExceededError",
    "InternalError",
    "HabitNotFoundError",
    "HabitAlreadyExistsError",
    "InvalidHabitIdError",
    "HabitTrackerError",
    "check_status",
]
