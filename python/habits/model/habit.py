"""Habit model representing a single habit's data."""
class Habit:
    """Represents a single habit with all its tracking data."""
    def __init__(self, habit_id, name, description, status, category,
                 current_streak, best_streak, total_checkins, last_checkin):

        self.habit_id = habit_id
        self.name = name
        self.description = description
        self.status = status
        self.category = category
        self.current_streak = current_streak
        self.best_streak = best_streak
        self.total_checkins = total_checkins
        self.last_checkin = last_checkin

    def to_dict(self):
        return {
            "id": self.habit_id,
            "name": self.name,
            "description": self.description,
            "status": self.status,
            "category": self.category,
            "current_streak": self.current_streak,
            "best_streak": self.best_streak,
            "total_checkins": self.total_checkins,
            "last_checkin": self.last_checkin,
        }