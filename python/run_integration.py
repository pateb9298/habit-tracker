"""
Integration test script: exercises the full habit tracker system 
(C backend through Python bindings) and prints rendered output.
"""
from habits.habit_tracker import HabitTracker
import os

CONFIG_PATH = os.path.join(os.path.dirname(__file__), "..", "assets", "datagen.ini")


def main():
    """Create a tracker, add a habit, check in, and print its render."""
    tracker = HabitTracker(CONFIG_PATH)

    try:
        habit_id = tracker.add_habit("Exercise", "30 min walk", (2026, 1, 1))
        tracker.check_in_habit(habit_id, (2026, 1, 1))

        print(tracker.render_habit(habit_id))

    finally:
        tracker.close()


if __name__ == "__main__":
    main()
