# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/chuck/Src/simp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/chuck/Src/simp/build

# Include any dependencies generated for this target.
include src/common/CMakeFiles/common.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/common/CMakeFiles/common.dir/compiler_depend.make

# Include the progress variables for this target.
include src/common/CMakeFiles/common.dir/progress.make

# Include the compile flags for this target's objects.
include src/common/CMakeFiles/common.dir/flags.make

src/common/CMakeFiles/common.dir/alloc.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/alloc.c.o: /home/chuck/Src/simp/src/common/alloc.c
src/common/CMakeFiles/common.dir/alloc.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/common/CMakeFiles/common.dir/alloc.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/alloc.c.o -MF CMakeFiles/common.dir/alloc.c.o.d -o CMakeFiles/common.dir/alloc.c.o -c /home/chuck/Src/simp/src/common/alloc.c

src/common/CMakeFiles/common.dir/alloc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/alloc.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/alloc.c > CMakeFiles/common.dir/alloc.c.i

src/common/CMakeFiles/common.dir/alloc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/alloc.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/alloc.c -o CMakeFiles/common.dir/alloc.c.s

src/common/CMakeFiles/common.dir/array.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/array.c.o: /home/chuck/Src/simp/src/common/array.c
src/common/CMakeFiles/common.dir/array.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object src/common/CMakeFiles/common.dir/array.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/array.c.o -MF CMakeFiles/common.dir/array.c.o.d -o CMakeFiles/common.dir/array.c.o -c /home/chuck/Src/simp/src/common/array.c

src/common/CMakeFiles/common.dir/array.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/array.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/array.c > CMakeFiles/common.dir/array.c.i

src/common/CMakeFiles/common.dir/array.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/array.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/array.c -o CMakeFiles/common.dir/array.c.s

src/common/CMakeFiles/common.dir/fileio.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/fileio.c.o: /home/chuck/Src/simp/src/common/fileio.c
src/common/CMakeFiles/common.dir/fileio.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object src/common/CMakeFiles/common.dir/fileio.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/fileio.c.o -MF CMakeFiles/common.dir/fileio.c.o.d -o CMakeFiles/common.dir/fileio.c.o -c /home/chuck/Src/simp/src/common/fileio.c

src/common/CMakeFiles/common.dir/fileio.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/fileio.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/fileio.c > CMakeFiles/common.dir/fileio.c.i

src/common/CMakeFiles/common.dir/fileio.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/fileio.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/fileio.c -o CMakeFiles/common.dir/fileio.c.s

src/common/CMakeFiles/common.dir/hash.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/hash.c.o: /home/chuck/Src/simp/src/common/hash.c
src/common/CMakeFiles/common.dir/hash.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object src/common/CMakeFiles/common.dir/hash.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/hash.c.o -MF CMakeFiles/common.dir/hash.c.o.d -o CMakeFiles/common.dir/hash.c.o -c /home/chuck/Src/simp/src/common/hash.c

src/common/CMakeFiles/common.dir/hash.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/hash.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/hash.c > CMakeFiles/common.dir/hash.c.i

src/common/CMakeFiles/common.dir/hash.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/hash.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/hash.c -o CMakeFiles/common.dir/hash.c.s

src/common/CMakeFiles/common.dir/pointer_list.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/pointer_list.c.o: /home/chuck/Src/simp/src/common/pointer_list.c
src/common/CMakeFiles/common.dir/pointer_list.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object src/common/CMakeFiles/common.dir/pointer_list.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/pointer_list.c.o -MF CMakeFiles/common.dir/pointer_list.c.o.d -o CMakeFiles/common.dir/pointer_list.c.o -c /home/chuck/Src/simp/src/common/pointer_list.c

src/common/CMakeFiles/common.dir/pointer_list.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/pointer_list.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/pointer_list.c > CMakeFiles/common.dir/pointer_list.c.i

src/common/CMakeFiles/common.dir/pointer_list.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/pointer_list.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/pointer_list.c -o CMakeFiles/common.dir/pointer_list.c.s

src/common/CMakeFiles/common.dir/string_list.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/string_list.c.o: /home/chuck/Src/simp/src/common/string_list.c
src/common/CMakeFiles/common.dir/string_list.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object src/common/CMakeFiles/common.dir/string_list.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/string_list.c.o -MF CMakeFiles/common.dir/string_list.c.o.d -o CMakeFiles/common.dir/string_list.c.o -c /home/chuck/Src/simp/src/common/string_list.c

src/common/CMakeFiles/common.dir/string_list.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/string_list.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/string_list.c > CMakeFiles/common.dir/string_list.c.i

src/common/CMakeFiles/common.dir/string_list.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/string_list.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/string_list.c -o CMakeFiles/common.dir/string_list.c.s

src/common/CMakeFiles/common.dir/string_buffer.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/string_buffer.c.o: /home/chuck/Src/simp/src/common/string_buffer.c
src/common/CMakeFiles/common.dir/string_buffer.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object src/common/CMakeFiles/common.dir/string_buffer.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/string_buffer.c.o -MF CMakeFiles/common.dir/string_buffer.c.o.d -o CMakeFiles/common.dir/string_buffer.c.o -c /home/chuck/Src/simp/src/common/string_buffer.c

src/common/CMakeFiles/common.dir/string_buffer.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/string_buffer.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/string_buffer.c > CMakeFiles/common.dir/string_buffer.c.i

src/common/CMakeFiles/common.dir/string_buffer.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/string_buffer.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/string_buffer.c -o CMakeFiles/common.dir/string_buffer.c.s

src/common/CMakeFiles/common.dir/trace.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/trace.c.o: /home/chuck/Src/simp/src/common/trace.c
src/common/CMakeFiles/common.dir/trace.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object src/common/CMakeFiles/common.dir/trace.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/trace.c.o -MF CMakeFiles/common.dir/trace.c.o.d -o CMakeFiles/common.dir/trace.c.o -c /home/chuck/Src/simp/src/common/trace.c

src/common/CMakeFiles/common.dir/trace.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/trace.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/trace.c > CMakeFiles/common.dir/trace.c.i

src/common/CMakeFiles/common.dir/trace.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/trace.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/trace.c -o CMakeFiles/common.dir/trace.c.s

src/common/CMakeFiles/common.dir/cmdline.c.o: src/common/CMakeFiles/common.dir/flags.make
src/common/CMakeFiles/common.dir/cmdline.c.o: /home/chuck/Src/simp/src/common/cmdline.c
src/common/CMakeFiles/common.dir/cmdline.c.o: src/common/CMakeFiles/common.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object src/common/CMakeFiles/common.dir/cmdline.c.o"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/common/CMakeFiles/common.dir/cmdline.c.o -MF CMakeFiles/common.dir/cmdline.c.o.d -o CMakeFiles/common.dir/cmdline.c.o -c /home/chuck/Src/simp/src/common/cmdline.c

src/common/CMakeFiles/common.dir/cmdline.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/common.dir/cmdline.c.i"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/chuck/Src/simp/src/common/cmdline.c > CMakeFiles/common.dir/cmdline.c.i

src/common/CMakeFiles/common.dir/cmdline.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/common.dir/cmdline.c.s"
	cd /home/chuck/Src/simp/build/src/common && /usr/bin/clang $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/chuck/Src/simp/src/common/cmdline.c -o CMakeFiles/common.dir/cmdline.c.s

# Object files for target common
common_OBJECTS = \
"CMakeFiles/common.dir/alloc.c.o" \
"CMakeFiles/common.dir/array.c.o" \
"CMakeFiles/common.dir/fileio.c.o" \
"CMakeFiles/common.dir/hash.c.o" \
"CMakeFiles/common.dir/pointer_list.c.o" \
"CMakeFiles/common.dir/string_list.c.o" \
"CMakeFiles/common.dir/string_buffer.c.o" \
"CMakeFiles/common.dir/trace.c.o" \
"CMakeFiles/common.dir/cmdline.c.o"

# External object files for target common
common_EXTERNAL_OBJECTS =

/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/alloc.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/array.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/fileio.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/hash.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/pointer_list.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/string_list.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/string_buffer.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/trace.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/cmdline.c.o
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/build.make
/home/chuck/Src/simp/lib/libcommon.a: src/common/CMakeFiles/common.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/chuck/Src/simp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Linking C static library /home/chuck/Src/simp/lib/libcommon.a"
	cd /home/chuck/Src/simp/build/src/common && $(CMAKE_COMMAND) -P CMakeFiles/common.dir/cmake_clean_target.cmake
	cd /home/chuck/Src/simp/build/src/common && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/common.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/common/CMakeFiles/common.dir/build: /home/chuck/Src/simp/lib/libcommon.a
.PHONY : src/common/CMakeFiles/common.dir/build

src/common/CMakeFiles/common.dir/clean:
	cd /home/chuck/Src/simp/build/src/common && $(CMAKE_COMMAND) -P CMakeFiles/common.dir/cmake_clean.cmake
.PHONY : src/common/CMakeFiles/common.dir/clean

src/common/CMakeFiles/common.dir/depend:
	cd /home/chuck/Src/simp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chuck/Src/simp /home/chuck/Src/simp/src/common /home/chuck/Src/simp/build /home/chuck/Src/simp/build/src/common /home/chuck/Src/simp/build/src/common/CMakeFiles/common.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : src/common/CMakeFiles/common.dir/depend

