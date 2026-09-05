"""Curses UI for the habit tracker"""

import curses
from datetime import date

class HabitUI:
    """Handles display and input for the habit tracker interface.

    - draw() handles all rendering
    - handle_input() handles all keypresses
    - status_msg holds the current status bar message
    """

    def __init__(self, stdscr, controller, profile):
        self.stdscr = stdscr
        self.controller = controller
        self.profile = profile
        self.status_msg = ""

        # Currently selected habit index in the list
        self.selected_index = 0

        # The list of habits currently displayed
        self.habits = []
        
        # Hide the cursor
        curses.curs_set(0)

        # allow keypad mode so arrow keys work 
        self.stdscr.keypad(True)

        # Set up colors
        if curses.has_colors():
            curses.start_color()
            curses.init_pair(1, curses.COLOR_BLACK, curses.COLOR_WHITE) # selected habit
            curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_BLUE)  # background
            curses.init_pair(3, curses.COLOR_GREEN, curses.COLOR_BLUE) # active
            curses.init_pair(4, curses.COLOR_YELLOW, curses.COLOR_BLUE)  # inactive
            curses.init_pair(5, curses.COLOR_RED, curses.COLOR_BLUE)     # archived

        self.stdscr.bkgd(' ', curses.color_pair(2))
        
        # Load habits at startup
        self.habits = self.controller.get_all_habits()

    def draw(self):
        """Draw the full UI following the redraw loop. 

        1. Get data from controller
        2. Clear the buffer
        3. Draw each element
        4. Refresh
        """
        # Step 1: get data from controller
        # self.habits = self.controller.get_all_habits()

        # Step 2: clear the buffer
        self.stdscr.clear()

        # Get screen size every draw. Handles resizing safely
        height, width = self.stdscr.getmaxyx()

        # Step 3: draw each element
        self._draw_header(height, width)
        self._draw_habits(height, width)
        self._draw_keybindings(height, width)
        self._draw_dashboard(height, width)
        self.draw_status_bar(height, width)

        # Step 4: refresh
        self.stdscr.refresh()


    def _draw_header(self, height, width):
        """Draw the title bar at the top."""
        title = "Habit Tracker"
        self.stdscr.attron(curses.A_BOLD)
        self.stdscr.addstr(0, (width - len(title)) // 2, title)
        self.stdscr.attroff(curses.A_BOLD)
        self.stdscr.addstr(1, 0, "-" * (width - 1))

    def _draw_habits(self, height, width):
        """Draw the list of habits."""
        row = 2

        if not self.habits:
            self.stdscr.addstr(row, 0, "No habits found.")
            return
        
        for i, habit in enumerate(self.habits):
            # Stop drawing if we are getting too close to the bottom
            if row >= height - 6:
                break

            # Show > arrow for selected habit
            if i == self.selected_index:
                prefix = "> "
            else:
                prefix = "  "

            # Build the display line
            streak_info = f"streak: {habit.current_streak}"
            line = f"{prefix}{habit.name:<30} {habit.category:<12} {streak_info} [{habit.status}]"

            # Pick color based on status
            if habit.status == "active":
                pair = curses.color_pair(3)
            elif habit.status == "inactive":
                pair = curses.color_pair(4)
            else:
                pair = curses.color_pair(5)

            if i == self.selected_index:
                self.stdscr.addstr(row, 0, line[:width - 1], pair | curses.A_REVERSE)
            else:
                self.stdscr.addstr(row, 0, line[:width - 1], pair)

            row += 1

    def _draw_keybindings(self, height, width):
        """Draw the keybindings menu near the bottom."""
        row = height - 5
        self.stdscr.addstr(row, 0, "-" * (width - 1))
        keys = "[enter] complete  [a] add  e [edit] [d] delete/archive [c] category [s] search [f] filter [p] profile [x] export [i] import [q] quit"
        self.stdscr.addstr(row + 1, 0, keys[:width - 1])

    def _draw_dashboard(self, height, width):
        """Draw the daily progress dashboard."""
        stats = self.controller.get_dashboard_stats()
        row = height - 3
        self.stdscr.addstr(row, 0, "-" * (width - 1))
        dashboard = f"Career Summary | Total Habits: {stats['total']}  Completed today: {stats['completed_today']}  ({stats['completion_pct']}%)  Longest streak: {stats['longest_streak']}  Archived: {stats['archived']}"
        self.stdscr.addstr(row + 1, 0, dashboard[:width - 1])

    def draw_status_bar(self, height, width):
        """Draw status message at the bottom"""
        msg = self.status_msg[:width - 1]
        self.stdscr.attron(curses.A_REVERSE)
        self.stdscr.addstr(height - 1, 0, msg.ljust(width - 1))
        self.stdscr.attroff(curses.A_REVERSE)

    def handle_input(self, key):
        """Handle keypresses."""
        if key == ord('q'):
            self.controller.quit()
        
        elif key == curses.KEY_UP:
            if self.selected_index > 0:
                self.selected_index -= 1

        elif key == curses.KEY_DOWN:
            if self.selected_index < len(self.habits) - 1:
                self.selected_index += 1
        
        elif key == ord('\n'):
            # Enter key. check in selected habit for today
            self._do_checkin()

        elif key == ord('a'):
            self._do_add_habit()

        elif key == ord('e'):
            self._do_edit()

        elif key == ord('d'):
            self._do_archive_habit()

        elif key == ord('s'):
            self._do_search()
        
        elif key == ord('c'):
            self._do_set_category()
        
        elif key == ord('f'):
            self._do_filter_category()

        elif key == ord('p'):
            self._do_show_profile()

        elif key == ord('h'):
            self.status_msg = "Keys: up/down navigate, enter complete, a add, d archive/delete, c category, s search, f filter p profile, x export, i import, q quit"

        elif key == ord('x'):
            self._do_export()

        elif key == ord('i'):
            self._do_import()

    def _refresh_habits(self):
        """Reload habits from the controller."""
        self.habits = self.controller.get_all_habits()

    def _do_checkin(self):
        """Check in the selected habit for today."""
        if not self.habits:
            self.status_msg = "No habits to check in."
            return
        habit = self.habits[self.selected_index]

        if habit.status == "archived":
            self.status_msg = f"Cannot check in: {habit.name} is archived."
            return
        
        today = date.today()
        self.controller.check_in_habit(habit.habit_id, (today.year, today.month, today.day))
        self._refresh_habits()
        self.status_msg = f"Checked in: {habit.name}"

    def _do_add_habit(self):
        """Prompt user to add a new habit."""
        height, width = self.stdscr.getmaxyx()

        # Turn on echo so user can see what they type
        curses.echo()

        self.stdscr.addstr(height - 1, 0, "Habit name: ".ljust(width - 1))
        self.stdscr.refresh()
        name = self.stdscr.getstr(height - 1, 12, 50).decode("utf-8")

        self.stdscr.addstr(height - 1, 0, "Description: ".ljust(width - 1))
        self.stdscr.refresh()
        desc = self.stdscr.getstr(height - 1, 13, 100).decode("utf-8")

        # Turn echo back off
        curses.noecho()

        if name:
            today = date.today()
            self.controller.add_habit(name, desc, (today.year, today.month, today.day))
            self._refresh_habits()
            self.status_msg = f"Added: {name}"
        else:
            self.status_msg = "Cancelled."

    def _do_edit(self):
        """Prompt user to edit the selected habit's name and description."""
        if not self.habits:
            self.status_msg = "No habits to edit."
            return
        habit = self.habits[self.selected_index]

        if habit.status == "archived":
            self.status_msg = "Cannot edit an archived habit."
            return

        height, width = self.stdscr.getmaxyx()
        curses.echo()

        self.stdscr.addstr(height - 1, 0, f"New name ({habit.name}): ".ljust(width - 1))
        self.stdscr.refresh()
        name = self.stdscr.getstr(height - 1, len(f"New name ({habit.name}): "), 50).decode("utf-8")

        self.stdscr.addstr(height - 1, 0, f"New desc ({habit.description[:20]}): ".ljust(width - 1))
        self.stdscr.refresh()
        desc = self.stdscr.getstr(height - 1, len(f"New desc ({habit.description[:20]}): "), 100).decode("utf-8")

        curses.noecho()

        if name or desc:
            self.controller.edit_habit(
                habit.habit_id,
                name if name else habit.name,
                desc if desc else habit.description
            )
            self._refresh_habits()
            self.status_msg = f"Updated: {habit.name}"
        else:
            self.status_msg = "Cancelled."

    def _do_archive_habit(self):
        """Archive the selected habit."""
        if not self.habits:
            self.status_msg = "No habits to archive."
            return
        habit = self.habits[self.selected_index]
        self.controller.archive_habit(habit.habit_id)
        self._refresh_habits()
        self.status_msg = f"Archived: {habit.name}"

    def _do_search(self):
        """Prompt for search query and filter the habit list."""
        height, width = self.stdscr.getmaxyx()
        curses.echo()

        self.stdscr.addstr(height - 1, 0, "Search: ".ljust(width - 1))
        self.stdscr.refresh()
        query = self.stdscr.getstr(height - 1, 8, 50).decode("utf-8")

        curses.noecho()

        if query:
            self.habits = self.controller.search_habits(query)
            self.selected_index = 0
            self.status_msg = f"Results for: {query}"
        else:
            self._refresh_habits()
            self.status_msg = "Search cleared."

    def _do_set_category(self):
        """Prompt user to set category for selected habit."""
        if not self.habits:
            self.status_msg = "No habits to categorize."
            return
        height, width = self.stdscr.getmaxyx()
        curses.echo()

        categories = "Health/Exercise/School/Reading/Finance/Personal/General"
        self.stdscr.addstr(height - 1, 0, f"Category ({categories}): ".ljust(width - 1))
        self.stdscr.refresh()
        category = self.stdscr.getstr(height - 1, len(categories) + 12, 20).decode("utf-8")

        curses.noecho()

        if category:
            habit = self.habits[self.selected_index]
            self.controller.set_habit_category(habit.habit_id, category)
            self._refresh_habits()
            self.status_msg = f"Category set to: {category}"
        else:
            self.status_msg = "Cancelled."

    def _do_filter_category(self):
        """Prompt for category and filter the habit list."""
        height, width = self.stdscr.getmaxyx()
        curses.echo()

        self.stdscr.addstr(height - 1, 0, "Filter by category: ".ljust(width - 1))
        self.stdscr.refresh()
        query = self.stdscr.getstr(height - 1, 20, 50).decode("utf-8")

        curses.noecho()

        if query:
            self.habits = self.controller.filter_by_category(query)
            self.selected_index = 0
            self.status_msg = f"Filtered by: {query}"
        else:
            self._refresh_habits()
            self.status_msg = "Filter cleared."

    def _do_show_profile(self):
        """Show profile info in the status bar."""
        p = self.profile
        if p.last_login:
            last_login = p.last_login[:10]  # take YYYY-MM-DD part only
        else:
            last_login = "Never"

        self.status_msg = f"User: {p.user_name}  Sessions: {p.sessions}  Best streak: {p.highest_streak}  Last login: {last_login}"

    def _do_export(self):
        """Export all habits to JSON and CSV files."""
        json_path = self.controller.export_habits_json()
        csv_path = self.controller.export_habits_csv()
        self.status_msg = f"Exported to {json_path} and {csv_path}"

    def _do_import(self):
        """Prompt user for a file path and import habits from it."""
        height, width = self.stdscr.getmaxyx()
        curses.echo()

        self.stdscr.addstr(height - 1, 0, "Import file path: ".ljust(width - 1))
        self.stdscr.refresh()
        path = self.stdscr.getstr(height - 1, 18, 100).decode("utf-8")

        curses.noecho()

        if not path:
            self.status_msg = "Cancelled."
            return

        # try JSON first, then CSV based on file extension
        try:
            if path.endswith(".csv"):
                count = self.controller.import_habits_csv(path)
            else:
                count = self.controller.import_habits_json(path)
            self._refresh_habits()
            self.status_msg = f"Imported {count} habits from {path}"
        except FileNotFoundError:
            self.status_msg = f"File not found: {path}"
        except Exception:  # pylint: disable=broad-except
            self.status_msg = "Import failed — check file format."