"""Date model used by ctypes bindings and wrappers."""

from ctypes import Structure, c_int
# pylint: disable=too-few-public-methods

# Whenever C hands a Date or whenever I need to build one to pass to C
# Here is exactly what it looks like in memory, three ints
class Date(Structure):
    """C Date structure."""

    _fields_ = [
        ("year", c_int),
        ("month", c_int),
        ("day", c_int),
    ]

    def __repr__(self):
        return f"Date({self.year:04d}-{self.month:02d}-{self.day:02d})"
