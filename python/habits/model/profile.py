""" User profile model for long-term statustics"""

import json
from pathlib import Path
from datetime import datetime, timezone # for recording the current login time

# Path to profile.json relative to this file
# Goes up two levels from model/ to python/, then into assets/
PROFILE_PATH = Path(__file__).parent.parent.parent / "assets" / "profile.json"

class Profile:
    """ Manages user profile data persisted to JSON."""

    def __init__(self, path=None):
        # Use the provided path or fall back to the default PROFILE_PATH above
        if path:
            self.path = Path(path)
        else:
            self.path = PROFILE_PATH

        # Load profile from disk, or get defaults if file does not exist.
        data = self._load()

        # These are the default values if no profile.json exists yet
        self.user_name = data.get("user_name", "User")
        self.sessions = data.get("sessions", 0)
        self.habits_completed = data.get("habits_completed", 0)
        self.highest_streak = data.get("highest_streak", 0)
        self.favorite_category = data.get("favorite_category", "None")
        self.last_login = data.get("last_login", None)

    def _load(self):
        """Load profile from JSON file, return defaults if file doesn't exist."""
        if self.path.exists():
            with open(self.path) as f:
                return json.load(f)

        # File not found — return defaults
        return {
            "user_name": "User",
            "sessions": 0,
            "habits_completed": 0,
            "highest_streak": 0,
            "favorite_category": "None",
            "last_login": None
        }

    def _save(self):
        """Write profile back to disk."""
         
        # Create the assets folder if it doesn't exist yet
        self.path.parent.mkdir(parents=True, exist_ok=True)

        with open(self.path, "w") as f:
            json.dump(self.to_dict(), f, indent=2)

    def start_session(self):
        """Increment session count and record login time."""
        self.sessions += 1
        self.last_login = datetime.now(timezone.utc).isoformat()
        self._save()

    def update_stats(self, habits_completed=0, highest_streak=0, favorite_category=None):
        """Update stats and save."""
        self.habits_completed += habits_completed
        if highest_streak > self.highest_streak:
            self.highest_streak = highest_streak
        if favorite_category:
            self.favorite_category = favorite_category
        self._save()

    def to_dict(self):
        """Return profile as a dictionary (lecture slide 20 pattern)."""
        return {
            "user_name": self.user_name,
            "sessions": self.sessions,
            "habits_completed": self.habits_completed,
            "highest_streak": self.highest_streak,
            "favorite_category": self.favorite_category,
            "last_login": self.last_login,
        }