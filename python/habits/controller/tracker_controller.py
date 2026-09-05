"""Controller for the habit tracker. Coordinates model and view."""

from habits.habit_tracker import HabitTracker
from habits.model.habit import Habit

import json
import csv
import os
from datetime import date

class TrackerController:
    """Coordinates all user actions between the view and the model."""

    def __init__(self, config_path):
        # Create the habit tracker (this loads habits from C)
        self.tracker = HabitTracker(config_path)
        self.running = True  

    def close(self):
        """Clean up the tracker when the app exits."""
        self.tracker.close()

    def get_all_habits(self):
        """Get all habits as a list of Habit objects."""
        habits = []
        for info in self.tracker.get_all_habits():
            habit = Habit(
                habit_id=info["id"],
                name=info["name"],
                description=info["description"],
                status=info["status"],
                category=info["category"],
                current_streak=info["current_streak"],
                best_streak=info["best_streak"],
                total_checkins=info["total_checkins"],
                last_checkin=info["last_checkin"],
            )
            habits.append(habit)
        return habits

    def add_habit(self, name, description, date):
        """Add a new habit. date is a tuple (year, month, day)."""
        return self.tracker.add_habit(name, description, date)

    def check_in_habit(self, habit_id, date):
        """Check in for a habit on a given date."""
        self.tracker.check_in_habit(habit_id, date)

    def archive_habit(self, habit_id):
        """Archive a habit by ID."""
        self.tracker.archive_habit(habit_id)

    def search_habits(self, query):
        """Search habits by partial name match using C backend."""
        results = self.tracker.search_habits_by_name(query)
        habits = []
        for info in results:
            habit = Habit(
                habit_id=info["id"],
                name=info["name"],
                description=info["description"],
                status=info["status"],
                category=info["category"],
                current_streak=info["current_streak"],
                best_streak=info["best_streak"],
                total_checkins=info["total_checkins"],
                last_checkin=info["last_checkin"],
            )
            habits.append(habit)
        return habits

    def set_habit_category(self, habit_id, category):
        """Set the category for a habit."""
        self.tracker.set_habit_category(habit_id, category)

    def filter_by_category(self, category):
        """Filter habits by category."""
        all_habits = self.get_all_habits()
        return [h for h in all_habits if h.category == category]

    def get_dashboard_stats(self):
        """Return stats for the daily progress dashboard."""
        all_habits = self.get_all_habits()
        total = len(all_habits)
        archived = sum(1 for h in all_habits if h.status == "archived")
        longest_streak = max((h.current_streak for h in all_habits), default=0)
        completed_today = self.get_completed_today()
        
        # Calculate completion percentage based on non-archived habits
        active_habits = total - archived
        if active_habits > 0:
            completion_pct = int((completed_today / active_habits) * 100)
        else:
            completion_pct = 0

        return {
            "total": total,
            "archived": archived,
            "longest_streak": longest_streak,
            "completed_today": completed_today,
            "completion_pct": completion_pct,
        }

    def get_completed_today(self):
        """Return number of habits completed today."""
        return self.tracker.get_completed_today()

    def export_habits_json(self):
        """Export all habits to a JSON file.
    
        Gets all habits from C, converts each one to a dict,
        then writes the list of dicts to a JSON file.
        """

        # get all habits from C as Habit objects
        habits = self.get_all_habits()

        # Convert each Habit object to a dict using to_dict()
        data = [h.to_dict() for h in habits]

        # Build the path to the export file
        path = os.path.join(os.path.dirname(__file__), "../../assets/habits_export.json")
        path = os.path.abspath(path)

        # Create the assets folder if it does not exist
        os.makedirs(os.path.dirname(path), exist_ok=True)

        # Write the list of dicts to the JSON file
        with open(path, "w") as f:
            json.dump(data, f, indent=2)

        return path

    def export_habits_csv(self):
        """Export all habits to a CSV file.
    
        Gets all habits from C, writes each one as a row
        in a CSV file with a header row.
        """
        # Get all habits from C as habit objects
        habits = self.get_all_habits()

        # Build the path to the export file
        path = os.path.join(os.path.dirname(__file__), "../../assets/habits_export.csv")
        path = os.path.abspath(path)

        # create the assets folder if it doesn't exist
        os.makedirs(os.path.dirname(path), exist_ok=True)

        # Open the file for writing
        with open(path, "w", newline="") as f:
        # DictWriter writes dicts as rows, with a header row
            writer = csv.DictWriter(f, fieldnames=[
                "id", "name", "description", "status", "category",
                "current_streak", "best_streak", "total_checkins"
            ])
            # write the header row first
            writer.writeheader()

            # write one row per habit
            for habit in habits:
                writer.writerow({
                    "id": habit.habit_id,
                    "name": habit.name,
                    "description": habit.description,
                    "status": habit.status,
                    "category": habit.category,
                    "current_streak": habit.current_streak,
                    "best_streak": habit.best_streak,
                    "total_checkins": habit.total_checkins,
                })

        return path

    def import_habits_json(self, path):
        """Import habits from a JSON file.
    
        Reads a list of habit dicts from the file,
        then adds each one to the tracker using add_habit.
        
        Returns:
            int: number of habits successfully imported
        """

        # Check if file exists first
        if not os.path.exists(path):
            raise FileNotFoundError(f"File not found: {path}")

        # Read and parse the JSON file
        with open(path, "r") as f:
            data = json.load(f)

        # Keep track of how many were succesfully import
        imported = 0

        # Loop through each habit dict in the file
        for item in data:
            try:
                # Get the fields we need. Use defaults if missing
                name = item.get("name", "")
                description = item.get("description", "")
                category = item.get("category", "General")

                # Skip if name is empty
                if not name:
                    continue

                # Add the habit using today's date
                today = date.today()
                habit_id = self.add_habit(name, description, (today.year, today.month, today.day))

                # Set the category if we got one
                if category:
                    self.set_habit_category(habit_id, category)

                imported +=1 

            except Exception:  
                # skip habits that fail to import
                continue

        return imported

    def import_habits_csv(self, path):
        """Import habits from a CSV file.
    
        Reads each row from the CSV file,
        then adds each one to the tracker using add_habit.
        
        Returns:
            int: number of habits successfully imported
        """
        if not os.path.exists(path):
            raise FileNotFoundError(f"File not found: {path}")

        imported = 0

        # open the CSV file and read row by row
        with open(path, "r", newline="") as f:
            reader = csv.DictReader(f)

            for row in reader:
                try:
                    # get fields from the row
                    name = row.get("name", "")
                    description = row.get("description", "")
                    category = row.get("category", "General")

                    # skip if name is empty
                    if not name:
                        continue

                    # add the habit using today's date
                    today = date.today()
                    habit_id = self.add_habit(name, description, (today.year, today.month, today.day))

                    # set the category
                    if category:
                        self.set_habit_category(habit_id, category)

                    imported += 1

                except Exception:  
                    continue

        return imported

    def edit_habit(self, habit_id, new_name, new_desc):
        """Edit the name and description of a habit."""
        self.tracker.edit_habit(habit_id, new_name, new_desc)
                


    def quit(self):
        """Signal the main loop to stop."""
        self.running = False