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

# Include any dependencies generated for this target.
include third_party/clipper/CMakeFiles/clipper.dir/depend.make

# Include the progress variables for this target.
include third_party/clipper/CMakeFiles/clipper.dir/progress.make

# Include the compile flags for this target's objects.
include third_party/clipper/CMakeFiles/clipper.dir/flags.make

third_party/clipper/CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.o: third_party/clipper/CMakeFiles/clipper.dir/flags.make
third_party/clipper/CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.o: third_party/clipper/clipper_autogen/mocs_compilation.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jasonshiao/FLUX/swiftray/cmake-build-release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object third_party/clipper/CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.o"
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.o -c /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper/clipper_autogen/mocs_compilation.cpp

third_party/clipper/CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.i"
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper/clipper_autogen/mocs_compilation.cpp > CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.i

third_party/clipper/CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.s"
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper/clipper_autogen/mocs_compilation.cpp -o CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.s

third_party/clipper/CMakeFiles/clipper.dir/clipper.cpp.o: third_party/clipper/CMakeFiles/clipper.dir/flags.make
third_party/clipper/CMakeFiles/clipper.dir/clipper.cpp.o: ../third_party/clipper/clipper.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/jasonshiao/FLUX/swiftray/cmake-build-release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object third_party/clipper/CMakeFiles/clipper.dir/clipper.cpp.o"
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/clipper.dir/clipper.cpp.o -c /Users/jasonshiao/FLUX/swiftray/third_party/clipper/clipper.cpp

third_party/clipper/CMakeFiles/clipper.dir/clipper.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/clipper.dir/clipper.cpp.i"
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/jasonshiao/FLUX/swiftray/third_party/clipper/clipper.cpp > CMakeFiles/clipper.dir/clipper.cpp.i

third_party/clipper/CMakeFiles/clipper.dir/clipper.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/clipper.dir/clipper.cpp.s"
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/jasonshiao/FLUX/swiftray/third_party/clipper/clipper.cpp -o CMakeFiles/clipper.dir/clipper.cpp.s

# Object files for target clipper
clipper_OBJECTS = \
"CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.o" \
"CMakeFiles/clipper.dir/clipper.cpp.o"

# External object files for target clipper
clipper_EXTERNAL_OBJECTS =

third_party/clipper/libclipper.a: third_party/clipper/CMakeFiles/clipper.dir/clipper_autogen/mocs_compilation.cpp.o
third_party/clipper/libclipper.a: third_party/clipper/CMakeFiles/clipper.dir/clipper.cpp.o
third_party/clipper/libclipper.a: third_party/clipper/CMakeFiles/clipper.dir/build.make
third_party/clipper/libclipper.a: third_party/clipper/CMakeFiles/clipper.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/jasonshiao/FLUX/swiftray/cmake-build-release/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libclipper.a"
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && $(CMAKE_COMMAND) -P CMakeFiles/clipper.dir/cmake_clean_target.cmake
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/clipper.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
third_party/clipper/CMakeFiles/clipper.dir/build: third_party/clipper/libclipper.a

.PHONY : third_party/clipper/CMakeFiles/clipper.dir/build

third_party/clipper/CMakeFiles/clipper.dir/clean:
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper && $(CMAKE_COMMAND) -P CMakeFiles/clipper.dir/cmake_clean.cmake
.PHONY : third_party/clipper/CMakeFiles/clipper.dir/clean

third_party/clipper/CMakeFiles/clipper.dir/depend:
	cd /Users/jasonshiao/FLUX/swiftray/cmake-build-release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/jasonshiao/FLUX/swiftray /Users/jasonshiao/FLUX/swiftray/third_party/clipper /Users/jasonshiao/FLUX/swiftray/cmake-build-release /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper /Users/jasonshiao/FLUX/swiftray/cmake-build-release/third_party/clipper/CMakeFiles/clipper.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : third_party/clipper/CMakeFiles/clipper.dir/depend
