# Include common rules.
# Actually dont as the c Warning levels are too damn high
#include(../../cmake/common.cmake)
# Just include cuda_arch to suppress CMAKE 3.18 warnings on windows (where nvcc is used even when not neccesary)
get_filename_component(FLAMEGPU_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../ REALPATH)

# Set CUDA gencode arguments
include(${FLAMEGPU_ROOT}/cmake/cuda_arch.cmake)
# Set the C++ and CUDA standard to use
include(${FLAMEGPU_ROOT}/cmake/cxxstd.cmake)
# Get FLAMEGPU Version information
include(${FLAMEGPU_ROOT}/cmake/version.cmake)

# Python module name (must match module name in swig input *.i file)
SET(PYTHON_MODULE_NAME pyflamegpu)
message(STATUS "-----Configuring Project: ${PYTHON_MODULE_NAME}-----")

if(FLAMEGPU_VERSION_PYTHON_PUBLIC AND FLAMEGPU_VERSION_PYTHON_LOCAL)
  message(STATUS "${PYTHON_MODULE_NAME} version ${FLAMEGPU_VERSION_PYTHON_PUBLIC} (${FLAMEGPU_VERSION_PYTHON_LOCAL})")
endif()

# Set the output directory where the wheel etc should be placed.
# CMAKE 3.20 is required for byproducts contianing $<CONFIG> (i.e. multiconfig generators.) Workaround by only setting if supported, or usiing CMAKE_BUILD_TYPE for non-multi-generators.
# This is required for msvc multi-generator usage. For older cmake and non-mulit-config generators CMAKE_BUILD_TYPE can be used instead.
SET(PYTHON_LIB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>/python)
SET(PYTHON_LIB_OUTPUT_DIRECTORY_BYPRODUCTS TRUE)
if(${CMAKE_VERSION} VERSION_LESS "3.20")
  if(NOT ${GENERATOR_IS_MULTI_CONFIG})
    SET(PYTHON_LIB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}/python)
  else()
    SET(PYTHON_LIB_OUTPUT_DIRECTORY_BYPRODUCTS FALSE)
  endif()
endif()
# Python library temp directory (for autogenerated swig files etc.)
SET(PYTHON_LIB_TEMP_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# set input file name
SET(SWIG_INPUT_FILE_NAME flamegpu)

#########################################
## Swig input processing and building  ##
#########################################

# setup properties of swig input file
set_property(SOURCE ${SWIG_INPUT_FILE_NAME}.i PROPERTY CPLUSPLUS ON)
set_property(SOURCE ${SWIG_INPUT_FILE_NAME}.i PROPERTY SWIG_MODULE_NAME ${PYTHON_MODULE_NAME})
set_property(SOURCE ${SWIG_INPUT_FILE_NAME}.i PROPERTY SWIG_FLAGS "-threads")
#set_property(SOURCE ${SWIG_INPUT_FILE_NAME}.i PROPERTY SWIG_FLAGS "-builtin")

# Add swig module
swig_add_library(${PYTHON_MODULE_NAME}
	TYPE SHARED
	#TYPE STATIC
	LANGUAGE python 
	OUTPUT_DIR ${PYTHON_LIB_TEMP_DIRECTORY}/${PYTHON_MODULE_NAME}
	SOURCES ${SWIG_INPUT_FILE_NAME}.i)

# configure wrapper code to build with cuda compiler
set_source_files_properties(${PYTHON_LIB_TEMP_DIRECTORY}/${PYTHON_MODULE_NAME}/${SWIG_INPUT_FILE_NAME}PYTHON_wrap.cxx PROPERTIES LANGUAGE CUDA)

# use the target include directories
set_property(TARGET ${PYTHON_MODULE_NAME} PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON)
# needs to have RDC enable for the project
set_property(TARGET ${PYTHON_MODULE_NAME} PROPERTY CUDA_SEPARABLE_COMPILATION ON)
# Build with the SWIG pre-processor macro enabled
set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_GENERATED_COMPILE_DEFINITIONS "SWIG")
# Suppress warnings during compilation of SWIG generated PYTHON_wrap.cxx
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcudafe --diag_suppress=\"declared_but_not_referenced\"")
# set bigobj property for windows compiler (needed due to the sheer size of the generated swig file: https://www.reddit.com/r/AbsoluteUnits/)
if(MSVC)
	set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler /bigobj")
endif()
# set include directories for module build
target_include_directories(${PYTHON_MODULE_NAME}
  PRIVATE
  ${FLAMEGPU_ROOT}/include
  ${Python3_INCLUDE_DIRS}
  )
  
# Handle user options
if (VISUALISATION)
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_COMPILE_DEFINITIONS VISUALISATION)
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_GENERATED_COMPILE_DEFINITIONS VISUALISATION)
  target_include_directories(${PYTHON_MODULE_NAME} PUBLIC "${VISUALISATION_ROOT}/include")
endif()
if (SEATBELTS)
  # Debug alwasy has seatbelts.
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_COMPILE_DEFINITIONS SEATBELTS=1)
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_GENERATED_COMPILE_DEFINITIONS SEATBELTS=1)
else()
  # Debug builds still have seatbelts. Non debug builds do not.
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_COMPILE_DEFINITIONS $<IF:$<CONFIG:Debug>,SEATBELTS=1,SEATBELTS=0>)
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_GENERATED_COMPILE_DEFINITIONS $<IF:$<CONFIG:Debug>,SEATBELTS=1,SEATBELTS=0>)
endif()
if (NOT RTC_DISK_CACHE)
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_COMPILE_DEFINITIONS DISABLE_RTC_DISK_CACHE)
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_GENERATED_COMPILE_DEFINITIONS DISABLE_RTC_DISK_CACHE)
endif()    
if (EXPORT_RTC_SOURCES)
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_COMPILE_DEFINITIONS OUTPUT_RTC_DYNAMIC_FILES)
  set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_GENERATED_COMPILE_DEFINITIONS OUTPUT_RTC_DYNAMIC_FILES)
endif ()
# Block windows.h min and max defines.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_COMPILE_DEFINITIONS NOMINMAX)
    set_property(TARGET ${PYTHON_MODULE_NAME} APPEND PROPERTY SWIG_GENERATED_COMPILE_DEFINITIONS NOMINMAX)
endif()
  
# link with the static flamegpu and cuda libraries
target_link_libraries(${PYTHON_MODULE_NAME} PRIVATE flamegpu cuda)
# Variable PYTHON_LIBRARIES can contains keyword `optimized`
# which won't be interpreted inside a generator expression.
# i.e. we can't use: $<$<PLATFORM_ID:Windows>:${PYTHON_LIBRARIES}>
# see: https://cmake.org/cmake/help/git-stage/command/target_link_libraries.html#command:target_link_libraries
if(MSVC)
  target_link_libraries(${PYTHON_MODULE_NAME} PRIVATE ${Python3_LIBRARIES} flamegpu cuda)
endif()

#######################
## Python Packaging  ##
#######################
# cleanup the flamegpu include file paths, so they're relative (begin `include/`) and seperated by `", "`
foreach(FLAMEGPU_INC_FILE IN LISTS FLAMEGPU_INCLUDE)
  file(RELATIVE_PATH FLAMEGPU_INC_FILE_CLEAN "${FLAMEGPU_ROOT}" "${FLAMEGPU_INC_FILE}")
  set(FLAMEGPU_INCLUDE_CLEAN "${FLAMEGPU_INCLUDE_CLEAN}'${FLAMEGPU_INC_FILE_CLEAN}', ")
endforeach()

# Build a list of OS specific python package_data entries.
set(FLAMEGPU_PYTHON_PACKAGE_DATA_OS_SPECIFIC "")
if(WIN32)
  # @todo - See if there is a dynamic way to achieve this through target properties?  
  set(FLAMEGPU_PYTHON_PACKAGE_DATA_OS_SPECIFIC "${FLAMEGPU_PYTHON_PACKAGE_DATA_OS_SPECIFIC}'glew32.dll', 'SDL2.dll', 'DevIL.dll'")
endif()


# Get the cuda major and minor versions as required for setup.py. There is no easy cmake way to access this that is supported across multiple CMake versions..
string(REPLACE "." ";" CMAKE_CUDA_COMPILER_VERSION_LIST ${CMAKE_CUDA_COMPILER_VERSION})
list(GET CMAKE_CUDA_COMPILER_VERSION_LIST 0 CMAKE_CUDA_COMPILER_VERSION_MAJOR)
list(GET CMAKE_CUDA_COMPILER_VERSION_LIST 1 CMAKE_CUDA_COMPILER_VERSION_MINOR)


# configure the python setup.py and __init__.py files for packaging and output to the final lib output folder
configure_file(
	setup.py.in
	${PYTHON_LIB_TEMP_DIRECTORY}/setup.py.in
	@ONLY)
unset(CMAKE_CUDA_COMPILER_VERSION_LIST)
unset(CMAKE_CUDA_COMPILER_VERSION_MAJOR)
unset(CMAKE_CUDA_COMPILER_VERSION_MINOR)
unset(FLAMEGPU_PYTHON_PACKAGE_DATA_OS_SPECIFIC)

file(GENERATE
	OUTPUT ${PYTHON_LIB_OUTPUT_DIRECTORY}/setup.py
	INPUT ${PYTHON_LIB_TEMP_DIRECTORY}/setup.py.in)
configure_file(
	__init__.py.in
	${PYTHON_LIB_TEMP_DIRECTORY}/__init__.py.in
	@ONLY)
file(GENERATE
	OUTPUT ${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}/__init__.py
	INPUT ${PYTHON_LIB_TEMP_DIRECTORY}/__init__.py.in)

# Function to find if python module MODULE_NAME is available, if not then install it to the Python user install directory.
function(search_python_module MODULE_NAME)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import ${MODULE_NAME}; print(${MODULE_NAME}.__version__) if hasattr(${MODULE_NAME}, '__version__') else print('Unknown');"
    RESULT_VARIABLE _RESULT
    OUTPUT_VARIABLE MODULE_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(${_RESULT} STREQUAL "0")
    message(STATUS "Found python module: ${MODULE_NAME} (version \"${MODULE_VERSION}\")")
  else()
    message(WARNING "Can't find python module \"${MODULE_NAME}\", user install it using pip...")
    execute_process(
      COMMAND ${Python3_EXECUTABLE} -m pip install --upgrade --user ${MODULE_NAME}
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  endif()
endfunction()

# Look for required python modules to build the python module
# @todo - Python3.5 is no longer supported by setuptools > 51.3, so need to change the install command to reflect this in cases where pip is too old (i.e. ubuntu 16.04 on CI). It would be better to not install dependencies into non-venvs too.
# Arguably, we could set up a requirements.txt for building (and exeucting)?
# https://github.com/pypa/setuptools/issues/2541
search_python_module(setuptools)
search_python_module(wheel)

# Create the package directory before we try and copy files to it
add_custom_command(TARGET ${PYTHON_MODULE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}/include")

# Copy the FLAMEGPU headers to a local path, this must occur before the wheel is built
add_custom_command(TARGET ${PYTHON_MODULE_NAME} POST_BUILD     # Adds a post-build event to MyTest
    COMMAND ${CMAKE_COMMAND} -E copy_directory              # which executes "cmake - E copy_directory..."
        "${FLAMEGPU_ROOT}/include"                                     # <--this is in-dir
        "${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}/include" # <--this is out-dir path
    ) 
# Copy the visualisation dlls if required, this must occur before the wheel is built
if (VISUALISATION)
    # Copy DLLs
    if(WIN32)
        # These might fall over if libraries becomes plural
        get_filename_component(SDL2_RUNTIME_LIB_NAME ${SDL2_RUNTIME_LIBRARIES} NAME)
        get_filename_component(GLEW_RUNTIME_LIB_NAME ${GLEW_RUNTIME_LIBRARIES} NAME)
        get_filename_component(IL_RUNTIME_LIB_NAME ${IL_RUNTIME_LIBRARIES} NAME)
        #get_filename_component(ILUT_RUNTIME_LIB_NAME ${ILUT_RUNTIME_LIBRARIES} NAME)
        # sdl 
        set(SDL_BYPRODUCTS "${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}/${SDL2_RUNTIME_LIB_NAME}")
        if(NOT PYTHON_LIB_OUTPUT_DIRECTORY_BYPRODUCTS)
          set(SDL_BYPRODUCTS "")
        endif()
        add_custom_command(TARGET ${PYTHON_MODULE_NAME} POST_BUILD     # Adds a post-build event to MyTest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different              # which executes "cmake - E copy_if_different..."
                "${SDL2_RUNTIME_LIBRARIES}"                            # <--this is in-file
                "${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}" # <--this is out-file path
            BYPRODUCTS "${SDL_BYPRODUCTS}")
        # glew
        set(GLEW_BYPRODUCTS "${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}/${GLEW_RUNTIME_LIB_NAME}")
        if(NOT PYTHON_LIB_OUTPUT_DIRECTORY_BYPRODUCTS)
          set(GLEW_BYPRODUCTS "")
        endif()
        add_custom_command(TARGET ${PYTHON_MODULE_NAME} POST_BUILD     # Adds a post-build event to MyTest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different              # which executes "cmake - E copy_if_different..."
                "${GLEW_RUNTIME_LIBRARIES}"                            # <--this is in-file
                "${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}" # <--this is out-file path
            BYPRODUCTS "${GLEW_BYPRODUCTS}")
        # DevIL
        set(IL_BYPRODUCTS "${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}/${IL_RUNTIME_LIB_NAME}")
        if(NOT PYTHON_LIB_OUTPUT_DIRECTORY_BYPRODUCTS)
          set(IL_BYPRODUCTS "")
        endif()
        add_custom_command(TARGET ${PYTHON_MODULE_NAME} POST_BUILD     # Adds a post-build event to MyTest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different              # which executes "cmake - E copy_if_different..."
                ${IL_RUNTIME_LIBRARIES}                                # <--this is in-file
                "${PYTHON_LIB_OUTPUT_DIRECTORY}/${PYTHON_MODULE_NAME}" # <--this is out-file path
            BYPRODUCTS "${IL_BYPRODUCTS}")
    endif()
endif()

# Configure and build the python package
# The file configurations above will already have output setup.py and __init__.py to the appropriate places and created the directory for the module
add_custom_command(TARGET ${PYTHON_MODULE_NAME} POST_BUILD
	# copy generated python file to library output location
	COMMAND ${CMAKE_COMMAND} -E copy ${PYTHON_LIB_TEMP_DIRECTORY}/${PYTHON_MODULE_NAME}/${PYTHON_MODULE_NAME}.py ${PYTHON_MODULE_NAME}
	# copy the compiled pyd file to library output location
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PYTHON_MODULE_NAME}> ${PYTHON_MODULE_NAME}
	COMMAND ${Python3_EXECUTABLE} setup.py bdist_wheel
	# by products of the packaging
	BYPRODUCTS
		${PYTHON_MODULE_NAME}
		build
		dist
		${PYTHON_MODULE_NAME}.egg-info
		setup.py
	WORKING_DIRECTORY ${PYTHON_LIB_OUTPUT_DIRECTORY}
)

# Build Virtual Environment for python testing and install the packaged wheel
if(BUILD_SWIG_PYTHON_VENV)
	# Look for python module venv
	search_python_module(venv)
	# Testing using a virtual environment
	set(VENV_EXECUTABLE ${Python3_EXECUTABLE} -m venv)
	set(VENV_DIR ${PYTHON_LIB_OUTPUT_DIRECTORY}/venv)
  set(VENV_BYPRODUCTS "${VENV_DIR}")
  if(NOT PYTHON_LIB_OUTPUT_DIRECTORY_BYPRODUCTS)
    set(VENV_BYPRODUCTS "")
  endif()
	if(WIN32)
		set(VENV_PIP "${VENV_DIR}\\Scripts\\pip.exe")
	else()
		set(VENV_PIP ${VENV_DIR}/bin/pip)
	endif()
	# make a venv to install our python package in it
	add_custom_command(TARGET ${PYTHON_MODULE_NAME} POST_BUILD
		# create venv
		COMMAND ${VENV_EXECUTABLE} ${VENV_DIR}

		# Must not call it in a folder containing the setup.py otherwise pip call it
		# (i.e. "python setup.py bdist") while we want to consume the wheel package
		COMMAND ${VENV_PIP} install --force-reinstall --find-links=dist --no-index -U ${PYTHON_MODULE_NAME}
		# install pytest
		COMMAND ${VENV_PIP} install pytest
		BYPRODUCTS ${VENV_BYPRODUCTS}
		# TODO: Add dependancy to ensure this post build step occurs after the previous post build step which builds the wheel
		WORKING_DIRECTORY ${PYTHON_LIB_OUTPUT_DIRECTORY})
	# TODO: Add tests (not supported by VS generator in CMAKE)
	#add_test(...)
endif()

# set project folders (for better organisation in visual studio)
if (CMAKE_USE_FOLDERS)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_property(TARGET "${PYTHON_MODULE_NAME}" PROPERTY FOLDER "FLAMEGPU")
endif ()