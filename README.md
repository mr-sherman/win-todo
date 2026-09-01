# TODO - A Terminal To-Do List Application

This is the home of the terminal application Todo.

## Purpose

The purpose of this application is to provide "to-do" list functionality to the terminal.

This started life as a Windows-native terminal application — many todo-management terminal apps exist for macOS, Linux, and other Unix environments, but using the popular ones on Windows usually means reaching for a virtualization layer (WSL) just to get a terminal tool. I wanted a Windows-native terminal application that doesn't require virtualization, containerization, or any other -ization other than organization.

It now builds and runs the same way on Linux and macOS too, using the same CMake + vcpkg setup either way — same source, same behavior, no WSL required on Windows and no extra hoops on Unix.

## Installation

A `.todo` folder is created in your home directory the first time you run the app:

- Windows: `%USERPROFILE%\.todo\`
- Linux/macOS: `$HOME/.todo/`

The SQLite database (`todo.db`) lives there. Right now, config options are rather minimal, but as this project may grow, the configuration options may grow. I've built this application for change.

## Getting Started

All you have to do to get started is to start creating entries for your todo list.

```sh
todo add <text>
```

```sh
todo list
```
Lists all your unresolved tasks.

```sh
todo complete <task number>
```
Finish the numbered task.

```sh
todo delete <task number>
```
Delete the task.

Note that there is a difference between `complete` and `delete`. Complete will keep the task in the database as complete for reporting purposes. Delete will remove it from the database and cannot be retrieved later for reporting purposes. Complete should be used for tasks that are done. Delete should be used for tasks that are abandoned.

### Export / Import

```sh
todo export <file>
```
Writes all open items to `<file>` as a markdown checklist:

```markdown
## Things to do for 2026-09-01

- [ ] buy milk <!-- id:1 -->
- [ ] write report <!-- id:2 -->
```

The `<!-- id:N -->` comment is invisible in any markdown viewer/renderer — it's how `import` maps a checked box back to the right database row. `N` is the row's stable SQLite rowid, not the task number shown by `list` (which shifts around every time something is completed or deleted), so checking boxes and importing works correctly no matter how many other tasks have been completed in the meantime, and importing the same file twice is harmless (already-closed items are silently skipped).

```sh
todo import <file>
```
Reads `<file>` as a markdown checklist and closes (marks complete) every item checked with `[x]` or `[X]`. Unchecked items are left alone. A checked item with no `<!-- id:N -->` comment (e.g. a checklist you wrote by hand rather than one `todo export` produced) can't be mapped to a database row, so it's reported to stderr and skipped rather than guessed at.

The intended workflow is `todo export todo.md`, check off what you finished in your editor, then `todo import todo.md`.

## Implementation

### Dependencies

vcpkg is the dependency manager for this project.

The database is SQLite. I used the `sqlitepp` header by Marco Paland as a C++ interface to the library.

`boost::program_options` is used for command-line arguments and configuration.

### Build System

This project uses CMake with a vcpkg manifest (`vcpkg.json`), so it builds the same way on Linux, macOS, and Windows.

#### Prerequisites

- CMake 3.16+
- A C++17 compiler (GCC, Clang, or MSVC)
- [vcpkg](https://github.com/microsoft/vcpkg)
- Ninja (optional but recommended on Linux/macOS)

#### 1. Get vcpkg

If you don't already have it:

```sh
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh      # Linux/macOS
.\vcpkg\bootstrap-vcpkg.bat     # Windows
```

#### 2. Configure and build

Dependencies (SQLite3, Boost's `program_options`) are fetched and built automatically the first time you configure, via the `vcpkg.json` manifest.

**Linux / macOS:**

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/todo list
```

**Windows (Developer PowerShell / cmd, with Visual Studio installed):**

```powershell
cmake -S . -B build ^
  -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
.\build\Release\todo.exe list
```

A `CMakePresets.json` is also included with `windows` and `unix` presets — set the `VCPKG_ROOT` environment variable and run `cmake --preset windows` (or `unix`) instead of typing out the toolchain file path each time. `CMakeUserPresets.json` is for your own machine-local overrides and is gitignored.

#### Installing

```sh
cmake --install build --prefix /usr/local
```

### Other Implementation Notes

I do not like object-oriented programming. I have some classes for convenience, but you won't see much in the way of design patterns, inheritance or encapsulation that doesn't make sense. If you ever see a `get_` method, in this code, I probably thought long and hard about it. But still question it and ask me why and possibly suggest a better way to do it.
