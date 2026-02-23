# Todo CLI - Daily Tasks and Habits Manager

A professional command-line application for managing daily tasks and habits with comprehensive progress tracking and visual analytics.

---

## PROJECT OVERVIEW

Todo CLI is an interactive terminal-based application that provides a clean, efficient interface for task and habit management without the need for complex graphical user interfaces. It offers:

### Key Features

- Separate Management for Tasks and Habits: Complete separation between regular tasks and daily/weekly habits
- Comprehensive Daily Tracking: Monitor completion levels daily with percentages and visual indicators
- Heatmap Visualization: Visual representation of your activity over previous days
- Tag System: Categorize tasks with colored tags like URGENT, DAILY, and WEEKLY
- Automatic Saving: All data is automatically saved in JSON format
- Fast and Lightweight Interface: C++ application that runs with exceptional speed
- Professional Design: Colorful, user-friendly interface with clear visual indicators

---

## QUICK START

### Requirements

- C++ (C++17 or later)
- CMake (version 3.15 or later)
- Standard development tools (gcc/clang and make)

### Installation and Build

```bash
# Navigate to the project directory
cd /home/dexter/todoCLI

# Create the build directory
mkdir -p build && cd build

# Build the project using CMake
cmake ..
make

# Run the application
./todo_app
```

---

## USAGE GUIDE

### Main Application Interface

The application is divided into three main sections:

```
+-----------------------------------------------------+
| TODO CLI v2.1.0 [CONNECTED_SESSION] [2025-02-23]   |
| .- Historical Heatmap (Last 8 Days) -.             |
| ===================================================
| TASKS > [=========       ] 3/5  60%
| +--------------------------------------------+
| 1. o Complete Report         [URGENT]       |
| 2. ✓ Review Email            [RUSH]         |
| 3. o Meeting at 3pm          [BUDGET]       |
| +--------------------------------------------+
|
| HABITS > [======         ] 2/6  33%
| +--------------------------------------------+
| 1. ✓ Exercise               [DAILY]         |
| 2. o Read 30 minutes        [DAILY]         |
| 3. ✓ Meditation             [WEEKLY]        |
| +--------------------------------------------+
|
| +--------------------------------------------+
| + COMMAND: [Enter task and press Enter...] |
| +--------------------------------------------+
| INPUT | Enter:add Tab:tasks Ctrl+L:habits Q:quit
+-----------------------------------------------------+
```

---

## KEYBOARD SHORTCUTS

### INPUT MODE

| Key | Function |
|-----|----------|
| Enter | Add a new task |
| Tab | Navigate to task list |
| Ctrl + L | Navigate to habits list |
| H | Show heatmap |
| Q | Exit application |

Important Notes:
- Type the task directly to add it as a regular task
- Type +H at the beginning to add a habit: Example "+H exercise"
- Add a tag with # at the end: Example "complete report #URGENT"

### TASKS MODE

| Key | Function |
|-----|----------|
| Up arrow or K | Move up |
| Down arrow or J | Move down |
| Space | Toggle task completion status |
| D | Delete selected task |
| Esc or Tab | Return to input |
| Ctrl + L | Navigate to habits |
| Q | Exit |

### HABITS MODE

| Key | Function |
|-----|----------|
| Up arrow or K | Move up |
| Down arrow or J | Move down |
| Space | Toggle habit completion status |
| D | Delete selected habit |
| Esc or Tab | Return to tasks |
| Tab | Navigate to tasks |
| Q | Exit |

---

## USAGE EXAMPLES

### Example 1: Adding Daily Tasks

```
+ COMMAND: complete report #URGENT
        Saved - appears in task list with red URGENT tag

+ COMMAND: client meeting at 2:00pm
        Saved as regular task

+ COMMAND: budget review #BUDGET
        Saved with green BUDGET tag
```

### Example 2: Adding Daily Habits

```
+ COMMAND: +H exercise #DAILY
        Saved as daily habit

+ COMMAND: +H read 30 minutes #DAILY
        Saved as daily habit

+ COMMAND: +H weekly goals review #WEEKLY
        Saved as weekly habit
```

### Example 3: Managing Tasks

```
1. Press Tab to navigate to task list
2. Use up/down arrows or J/K to move between tasks
3. Press Space to toggle task completion (✓ or o)
4. Press D to delete a task
5. Press Esc to return to input
```

---

## TAG SYSTEM

### Task Tags

| Tag | Color | Usage |
|-----|-------|-------|
| URGENT | Red | High priority, urgent tasks |
| RUSH | Orange | Quick priority tasks |
| BUDGET | Green | Financial or budget tasks |

### Habit Tags

| Tag | Color | Usage |
|-----|-------|-------|
| DAILY | Blue | Daily habits |
| WEEKLY | Purple | Weekly habits |

### Adding Custom Tags

```
+ COMMAND: learning new skill #LEARNING
        Available to add any custom tag
```

---

## VISUAL INDICATORS

### Task and Habit States

| Symbol | Meaning | Color |
|--------|---------|-------|
| o | Incomplete task | Yellow |
| ✓ | Completed task | Green |
| -- | Strikethrough (completed) | Gray |

### Progress Gauge

```
[========        ] 3/10  30%  Red (less than 30%)
[===========     ] 5/10  50%  Yellow (30% to 70%)
[================] 8/10  80%  Green (more than 70%)
```

---

## FILE STRUCTURE AND DATA

### Application Files

```
todoCLI/
├── main.cpp              # Main program source code
├── CMakeLists.txt        # Build configuration
├── README.md             # This file
├── CHANGES.md            # Historical changelog
├── build/                # Build directory (auto-created)
│   ├── todo_app          # Executable application
│   ├── todo_tasks.json   # Task data file
│   └── todo_habits.json  # Habit data file
└── _deps/               # External libraries
    └── ftxui/           # FTXUI interface library
```

### Data File Formats

**todo_tasks.json** - Contains daily tasks:
```json
{
  "days": {
    "2025-02-23": {
      "done": 2,
      "total": 5,
      "tasks": ["Complete report", "Meeting at 3", "Review email"],
      "task_done": [true, false, true],
      "task_tags": ["URGENT", "BUDGET", "RUSH"]
    },
    "2025-02-22": {
      "done": 4,
      "total": 4,
      "tasks": ["..."],
      "task_done": [true, true, true, true],
      "task_tags": ["", "", "", "URGENT"]
    }
  }
}
```

**todo_habits.json** - Contains daily habits:
```json
{
  "days": {
    "2025-02-23": {
      "done": 2,
      "total": 3,
      "tasks": ["Exercise", "Read 30 minutes", "Meditation"],
      "task_done": [true, false, true],
      "task_tags": ["DAILY", "DAILY", "WEEKLY"]
    }
  }
}
```

---

## HEATMAP VISUALIZATION

Shows a summary of your activity over the last 8 days:

```
  Mon       Tue       Wed       Thu       Fri       Sat       Sun       Today
  (Red)     (Orange)  (Yellow)  (Green)   (Blue)    (Purple)  (Gray)    (Gold)
```

Each square represents one day:
- Green: High completion rate (>70%)
- Yellow: Medium completion rate (30-70%)
- Red: Low completion rate (<30%)
- Gold: Current day

---

## ADVANCED TIPS

### For Higher Efficiency:

1. Use Tags Wisely
   - Set #URGENT for critical tasks
   - Use #DAILY and #WEEKLY for habits

2. Monitor Your Heatmap
   - Review your performance daily
   - Maintain a green level consistently

3. Work with Both Lists
   - Use Tab and Ctrl+L for quick navigation
   - Focus on important habits daily

4. Regular Saving
   - Data is saved automatically with each change
   - No need to worry about data loss

---

## TECHNICAL REQUIREMENTS

### Libraries Used

- FTXUI: Professional C++ library for building terminal user interfaces

### Supported Environments

- Linux: All distributions
- macOS: Intel and Apple Silicon
- Windows: With WSL or MSYS2

---

## CHANGELOG

To view all updates and new features:

```bash
cat CHANGES.md
```

---

## CONTRIBUTION AND SUPPORT

### To Report Issues or Suggest Features:

```bash
# Review the code
cat main.cpp

# Or modify files as needed
```

---

## LICENSE

This project is open source and available for personal and commercial use.

---

## QUICK REFERENCE

### Common Actions

| Action | Steps |
|--------|-------|
| Add regular task | Type task + Enter |
| Add habit | +H + task name + Enter |
| Complete task | Tab -> navigate up/down -> Space |
| Delete task | Tab -> navigate up/down -> D |
| Exit | Q from any mode |
| Show heatmap | H from input mode |

---

Last Updated: February 2025
Version: v2.1.0 HABITS
Status: Stable and ready for use
