"""Integration-style unit tests for the HabitTracker Python API."""
import unittest
import os
from habits.habit_tracker import HabitTracker
from habits.exceptions import HabitError


class TestHabitTracker(unittest.TestCase):
    """Tests exercising HabitTracker against the C backend."""
    def setUp(self):
        """Create a fresh tracker before each test."""
        self.config_path = os.path.join(
            os.path.dirname(__file__), "..", "..", "assets", "datagen.ini"
        )
        self.tracker = HabitTracker(self.config_path)

    def tearDown(self):
        """Close the tracker after each test."""
        self.tracker.close()

    def test_initial_habit_ids_loaded(self):
        """Loading the config should produce habit ids [1, 2, 3]."""
        ids = self.tracker.get_habit_ids()
        self.assertEqual(ids, [1, 2, 3])

    def test_add_habit_returns_new_id(self):
        """Adding a habit should return the next incrementing id."""
        new_id = self.tracker.add_habit("Exercise", "30 min walk", (2026, 1, 1))
        self.assertEqual(new_id, 4)

    def test_check_in_and_render_habit(self):
        """Check-in should be reflected in the rendered habit output."""
        new_id = self.tracker.add_habit("Read", "Read a book", (2026, 1, 1))
        self.tracker.check_in_habit(new_id, (2026, 1, 1))
        rendered = self.tracker.render_habit(new_id)
        self.assertIn(f"ID: {new_id}", rendered)
        self.assertIn("Name: Read", rendered)
        self.assertIn("Current streak: 1", rendered)

    def test_get_habit_info_existing_habit(self):
        """get_habit_info should return a dict with expected keys."""
        info = self.tracker.get_habit_info(3)
        self.assertEqual(info["id"], 3)
        self.assertIn("name", info)
        self.assertIn("status", info)

    # Exception & error-path tests
    def test_check_in_invalid_habit_id_raises(self):
        """Checking in an unknown habit id should raise HabitError."""
        with self.assertRaises(HabitError):
            self.tracker.check_in_habit(9999, (2026, 1, 1))

    def test_archive_and_status(self):
        """Archiving a habit should change its reported status."""
        new_id = self.tracker.add_habit("Meditate", "10 min", (2026, 1, 1))
        self.tracker.archive_habit(new_id)
        status = self.tracker.get_habit_status(new_id)
        self.assertEqual(status, "archived")

    # reset() test
    def test_reset_reloads_original_habits(self):
        """Resetting should discard added habits and reload the config."""
        self.tracker.add_habit("Temp", "temp desc", (2026, 1, 1))
        self.tracker.reset()
        ids = self.tracker.get_habit_ids()
        self.assertEqual(ids, [1, 2, 3])


if __name__ == "__main__":
    unittest.main()
