# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.19

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Disable VCS-based implicit rules.
% : %,v


# Disable VCS-based implicit rules.
% : RCS/%


# Disable VCS-based implicit rules.
% : RCS/%,v


# Disable VCS-based implicit rules.
% : SCCS/s.%


# Disable VCS-based implicit rules.
% : s.%


.SUFFIXES: .hpux_make_needs_suffix_list


# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/jasonshiao/FLUX/swiftray

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/jasonshiao/FLUX/swiftray/cmake-build-release

# Utility rule file for clipper_autogen.

# Include the progress variables for this target.
include third_party/clipper/CMakeFiles/clipper_autogen.dir/progress.make

third_party/clipper/CMakeFiles/clipper_autogen:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/Users/jasonshiao/FLUX/swiftray/cmake-build-release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Automatic MOC and UIC for target clipper"
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E cmake_autogen /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper/CMakeFiles/clipper_autogen.dir/AutogenInfo.json Release

clipper_autogen: third_party/clipper/CMakeFiles/clipper_autogen
clipper_autogen: third_party/clipper/CMakeFiles/clipper_autogen.dir/build.make

.PHONY : clipper_autogen

# Rule to build all files generated by this target.
third_party/clipper/CMakeFiles/clipper_autogen.dir/build: clipper_autogen

.PHONY : third_party/clipper/CMakeFiles/clipper_autogen.dir/build

third_party/clipper/CMakeFiles/clipper_autogen.dir/clean:
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && $(CMAKE_COMMAND) -P CMakeFiles/clipper_autogen.dir/cmake_clean.cmake
.PHONY : third_party/clipper/CMakeFiles/clipper_autogen.dir/clean

third_party/clipper/CMakeFiles/clipper_autogen.dir/depend:
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/jasonshiao/FLUX/swiftray /Users/jasonshiao/FLUX/swiftray/third_party/clipper /Users/jasonshiao/FLUX/swiftray/cmake-build-release /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper/CMakeFiles/clipper_autogen.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : third_party/clipper/CMakeFiles/clipper_autogen.dir/depend

