import curses
import os
from habits.controller.tracker_controller import TrackerController
from habits.view.ui_curses import HabitUI
from habits.model.profile import Profile

CONFIG_PATH = os.path.join(os.path.dirname(__file__), "..", "assets", "datagen.ini")

# Bhakti Look over !!!
def main(stdscr):
    # Load profile and start session
    profile = Profile()
    profile.start_session()

    # Create the controller 
    controller = TrackerController(CONFIG_PATH)

    # Create the UI
    ui = HabitUI(stdscr, controller, profile)
    ui.status_msg = "Press h for help, q to quit"

    while controller.running:
        ui.draw()
        key = stdscr.getch()
        ui.handle_input(key)

    # On exit, update profile stats and clean up
    profile.update_stats()
    controller.close()

curses.wrapper(main)