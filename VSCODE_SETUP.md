# VS Code Setup

This project includes a comprehensive VS Code configuration for debugging and development.

## Debug Configurations

The `.vscode/launch.json` file includes several debug configurations:

### Main Application
- **Debug BirdsOfPlay (Release Build)** - Debug the main application from release build
- **Debug BirdsOfPlay (Debug Build)** - Debug the main application from debug build

### Test Executables
- **Debug Tests (Release Build)** - Debug the main test suite
- **Debug Motion Tracker Tests** - Debug motion tracker specific tests
- **Debug Motion Processor Tests** - Debug motion processor specific tests

## Build Tasks

The `.vscode/tasks.json` file includes several build tasks:

### Build Tasks
- **C/C++: Build Release** (default) - Build project in release mode
- **C/C++: Build Debug** - Build project in debug mode
- **C/C++: Clean All** - Clean all build directories

### Test Tasks
- **C/C++: Run Tests** - Run all tests

## Usage

### Debugging
1. Open the project in VS Code
2. Set breakpoints in your code
3. Press `F5` or go to Run and Debug panel
4. Select the appropriate debug configuration
5. The debugger will automatically build the project and start debugging

### Building
- Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on macOS)
- Type "Tasks: Run Task"
- Select the desired build task

### Keyboard Shortcuts
- `F5` - Start debugging
- `Ctrl+Shift+P` - Command palette
- `Ctrl+Shift+B` - Build (runs default build task)

## Requirements

- VS Code with C/C++ extension
- Clang compiler (already configured)
- CMake (already configured)

## Notes

- All debug configurations use LLDB (macOS default debugger)
- Build tasks use the shell scripts in the root directory
- Working directories are set to the appropriate build directories
- Environment variables are configured for OpenCV logging
