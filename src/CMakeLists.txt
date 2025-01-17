# CMake 3.15+ for Thrust/Cub support
cmake_minimum_required(VERSION VERSION 3.15 FATAL_ERROR)

# Cmake 3.16 has issues with CUB and Thrust due to the order of the include path it generates.
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND CMAKE_VERSION VERSION_LESS 3.17)
    message(WARNING
    " CMake 3.16 does not use the correct CUDA include paths on some platforms.\n"
    " Please consider using a different version of CMake.")
endif()

# Set the location of the ROOT flame gpu project relative to this CMakeList.txt
get_filename_component(FLAMEGPU_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. REALPATH)

# See what languages are supported
# This is primarily here incase someone targets src/CMakeFiles.txt directly
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER STREQUAL NOTFOUND)
    project(flamegpu NONE)
    #Not able to build code, so just make docs
    message("Suitable compilers for building code not found.\n" "Attempting generation of minimal documentation only project.")
    include(${FLAMEGPU_ROOT}/cmake/doxygen.cmake)
    if(${BUILD_API_DOCUMENTATION})
        create_doxygen_target("${FLAMEGPU_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}" "")
    endif()
    return()
endif()
include(${FLAMEGPU_ROOT}/cmake/version.cmake)

# Name the project and set languages
project(flamegpu LANGUAGES CUDA CXX VERSION ${FLAMEGPU_VERSION})

# Option to enable/disable runtime checks which may impact performance
# This will primarily prevent device code from reporting errors
option(SEATBELTS "Enable runtime checks which harm performance for release/profile builds.\nThis should only be disabled after a model is known to be correct." ON)

# Option to enable/disable the default status of JitifyCache
option(RTC_DISK_CACHE "Enable caching of RTC kernels to disk by default (this can still be overridden programatically)." ON)

# Option to make put glm on the include path
option(USE_GLM "Experimental: Make GLM available to flamegpu2 projects on the include path" OFF)
mark_as_advanced(USE_GLM)

# Include common rules.
include(${FLAMEGPU_ROOT}/cmake/common.cmake)
include(${FLAMEGPU_ROOT}/cmake/doxygen.cmake)

if(VISUALISATION)
  include(${FLAMEGPU_ROOT}/cmake/flamegpu2-visualiser.cmake)
endif()

# Output the full FLAMEGPU version
if(FLAMEGPU_VERSION_FULL)
    message(STATUS "${PROJECT_NAME} version ${FLAMEGPU_VERSION_FULL}")
endif()

# Configure the version header file. Ideally this would configure a .cpp file in the build directory, but that doesn't allow for rtc header include checking sub major/minor/patch.
set(DYNAMIC_VERSION_HEADER_SRC "${FLAMEGPU_ROOT}/cmake/version.h.in")
set(DYNAMIC_VERSION_HEADER_DEST "${FLAMEGPU_ROOT}/include/flamegpu/version.h")
configure_file(${DYNAMIC_VERSION_HEADER_SRC} ${DYNAMIC_VERSION_HEADER_DEST})

# Define output location of static library
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    # If top level project
    SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}/)
    if(${BUILD_API_DOCUMENTATION})
        create_doxygen_target("${FLAMEGPU_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}" "")
    endif()
else()
    # If called via add_subdirectory()
    SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../lib/${CMAKE_BUILD_TYPE}/)
    if(${BUILD_API_DOCUMENTATION})
        create_doxygen_target("${FLAMEGPU_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/.." "")
    endif()
endif()

# Enable parallel compilation
# Only do this for lib, as examples already build target parallel
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
endif()

# Specify source files for the static library
# Can't do this automatically, as CMake wouldn't know when to regen (as CMakeLists.txt would be unchanged)
# version.h is automatically generated so does not exist prior to configure time
SET(SRC_INCLUDE
    ${DYNAMIC_VERSION_HEADER_DEST}
    ${FLAMEGPU_ROOT}/include/flamegpu/defines.h
    ${FLAMEGPU_ROOT}/include/flamegpu/flamegpu.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/StateReader.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/StateWriter.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/JSONStateReader.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/JSONStateWriter.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/XMLStateReader.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/XMLStateWriter.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/StateReaderFactory.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/StateWriterFactory.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/Logger.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/LoggerFactory.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/XMLLogger.h
    ${FLAMEGPU_ROOT}/include/flamegpu/io/JSONLogger.h
    ${FLAMEGPU_ROOT}/include/flamegpu/exception/FLAMEGPUException.h
    ${FLAMEGPU_ROOT}/include/flamegpu/exception/FLAMEGPUDeviceException.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/exception/FLAMEGPUDeviceException_device.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/model/LayerDescription.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/ModelData.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/AgentData.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/AgentFunctionData.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/model/DependencyNode.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/DependencyGraph.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/LayerData.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/AgentFunctionDescription.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/HostFunctionDescription.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/EnvironmentDescription.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/AgentDescription.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/ModelDescription.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/Variable.h
    ${FLAMEGPU_ROOT}/include/flamegpu/pop/detail/MemoryVector.h
    ${FLAMEGPU_ROOT}/include/flamegpu/pop/detail/GenericMemoryVector.h
    ${FLAMEGPU_ROOT}/include/flamegpu/pop/AgentVector.h
    ${FLAMEGPU_ROOT}/include/flamegpu/pop/AgentVector_Agent.h
    ${FLAMEGPU_ROOT}/include/flamegpu/pop/AgentInstance.h
    ${FLAMEGPU_ROOT}/include/flamegpu/pop/DeviceAgentVector.h
    ${FLAMEGPU_ROOT}/include/flamegpu/pop/DeviceAgentVector_impl.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAScanCompaction.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/detail/CUDAErrorChecking.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAMessageList.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDASimulation.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAEnsemble.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAMessage.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAAgent.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAAgentStateList.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAFatAgent.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAFatAgentStateList.h
    ${FLAMEGPU_ROOT}/include/flamegpu/gpu/CUDAScatter.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/AgentInterface.h
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/AgentLoggingConfig.h
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/AgentLoggingConfig_Reductions.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/LoggingConfig.h
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/LogFrame.h
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/RunPlan.h
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/RunPlanVector.h
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/SimRunner.h
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/SimLogger.h
    ${FLAMEGPU_ROOT}/include/flamegpu/sim/Simulation.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/AgentFunction.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/AgentFunction_shim.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/AgentFunctionCondition.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/AgentFunctionCondition_shim.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/HostFunctionCallback.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/DeviceAPI.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/HostAPI.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/HostAPI_macros.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/HostAgentAPI.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/HostNewAgentAPI.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/detail/curve/curve.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/detail/curve/curve_rtc.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging_device.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageSpecialisationHandler.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageNone.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageNone/MessageNoneHost.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageNone/MessageNoneDevice.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageBruteForce.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageBruteForce/MessageBruteForceDevice.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageBruteForce/MessageBruteForceHost.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageSpatial2D.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageSpatial2D/MessageSpatial2DHost.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageSpatial2D/MessageSpatial2DDevice.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageSpatial3D.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageSpatial3D/MessageSpatial3DHost.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageSpatial3D/MessageSpatial3DDevice.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray/MessageArrayHost.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray/MessageArrayDevice.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray2D.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray2D/MessageArray2DHost.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray2D/MessageArray2DDevice.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray3D.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray3D/MessageArray3DHost.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageArray3D/MessageArray3DDevice.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageBucket.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageBucket/MessageBucketHost.h
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/messaging/MessageBucket/MessageBucketDevice.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/utility/AgentRandom.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/utility/DeviceEnvironment.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/utility/EnvironmentManager.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/utility/HostEnvironment.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/utility/HostRandom.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/runtime/utility/RandomManager.cuh    
    ${FLAMEGPU_ROOT}/include/flamegpu/util/Any.h
    ${FLAMEGPU_ROOT}/include/flamegpu/util/nvtx.h
    ${FLAMEGPU_ROOT}/include/flamegpu/util/StringPair.h
    ${FLAMEGPU_ROOT}/include/flamegpu/util/StringUint32Pair.h
    ${FLAMEGPU_ROOT}/include/flamegpu/util/detail/compute_capability.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/util/detail/CUDAEventTimer.cuh
    ${FLAMEGPU_ROOT}/include/flamegpu/util/detail/cxxname.hpp
    ${FLAMEGPU_ROOT}/include/flamegpu/util/detail/filesystem.h
    ${FLAMEGPU_ROOT}/include/flamegpu/util/detail/SignalHandlers.h
    ${FLAMEGPU_ROOT}/include/flamegpu/util/detail/StaticAssert.h
    ${FLAMEGPU_ROOT}/include/flamegpu/util/detail/SteadyClockTimer.h
    ${FLAMEGPU_ROOT}/include/flamegpu/util/detail/JitifyCache.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/SubModelData.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/SubAgentData.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/SubEnvironmentData.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/SubModelDescription.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/SubAgentDescription.h
    ${FLAMEGPU_ROOT}/include/flamegpu/model/SubEnvironmentDescription.h
)
SET(SRC_FLAMEGPU
    ${FLAMEGPU_ROOT}/src/flamegpu/exception/FLAMEGPUException.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/exception/FLAMEGPUDeviceException.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/model/LayerDescription.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/ModelData.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/AgentData.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/AgentFunctionData.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/LayerData.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/ModelDescription.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/EnvironmentDescription.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/model/AgentDescription.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/AgentFunctionDescription.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/HostFunctionDescription.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/DependencyNode.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/DependencyGraph.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/pop/AgentVector.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/pop/AgentVector_Agent.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/pop/AgentInstance.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/pop/DeviceAgentVector_impl.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAScanCompaction.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAMessageList.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAAgent.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAAgentStateList.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAFatAgent.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAFatAgentStateList.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAMessage.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAScatter.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDASimulation.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/gpu/CUDAEnsemble.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/sim/AgentLoggingConfig.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/sim/LoggingConfig.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/sim/LogFrame.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/sim/RunPlan.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/sim/RunPlanVector.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/sim/SimRunner.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/sim/SimLogger.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/sim/Simulation.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/detail/curve/curve.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/detail/curve/curve_rtc.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/HostAPI.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/HostAgentAPI.cu 
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/messaging/MessageBruteForce.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/messaging/MessageSpatial2D.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/messaging/MessageSpatial3D.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/messaging/MessageArray.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/messaging/MessageArray2D.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/messaging/MessageArray3D.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/messaging/MessageBucket.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/io/JSONStateReader.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/io/JSONStateWriter.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/io/XMLStateReader.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/io/XMLStateWriter.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/io/XMLLogger.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/io/JSONLogger.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/utility/HostEnvironment.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/utility/EnvironmentManager.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/utility/RandomManager.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/runtime/utility/HostRandom.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/util/detail/compute_capability.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/util/detail/JitifyCache.cu
    ${FLAMEGPU_ROOT}/src/flamegpu/model/SubModelData.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/SubAgentData.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/SubEnvironmentData.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/SubModelDescription.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/SubAgentDescription.cpp
    ${FLAMEGPU_ROOT}/src/flamegpu/model/SubEnvironmentDescription.cpp
)
if (VISUALISATION)
    SET(SRC_INCLUDE
        ${SRC_INCLUDE}
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/visualiser_api.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/ModelVis.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/AgentVis.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/AgentStateVis.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/StaticModelVis.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/LineVis.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/color/Color.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/color/ColorFunction.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/color/DiscreteColor.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/color/StaticColor.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/color/HSVInterpolation.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/color/ViridisInterpolation.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/color/Palette.h
        ${FLAMEGPU_ROOT}/include/flamegpu/visualiser/color/AutoPalette.h
    )
    SET(SRC_FLAMEGPU
        ${SRC_FLAMEGPU}
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/ModelVis.cpp    
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/AgentVis.cpp     
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/AgentStateVis.cpp
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/StaticModelVis.cpp
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/LineVis.cpp
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/color/DiscreteColor.cpp
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/color/StaticColor.cpp
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/color/HSVInterpolation.cpp
        ${FLAMEGPU_ROOT}/src/flamegpu/visualiser/color/ViridisInterpolation.cpp
    )
endif()
SET(ALL_SRC
    ${SRC_INCLUDE}
    ${SRC_FLAMEGPU}
)
SET(FLAMEGPU_INCLUDE ${SRC_INCLUDE} CACHE INTERNAL "Include files required by FLAMEGPU RTC")

# Setup Visual Studio (and eclipse) filters
source_group(TREE ${FLAMEGPU_ROOT}/include/flamegpu PREFIX include FILES ${SRC_INCLUDE})
set(T_SRC "${SRC_FLAMEGPU}")
list(FILTER T_SRC INCLUDE REGEX ".*\.(h|hpp|cuh)$")
source_group(TREE ${FLAMEGPU_ROOT}/src/flamegpu PREFIX headers FILES ${T_SRC})
set(T_SRC "${SRC_FLAMEGPU}")
list(FILTER T_SRC EXCLUDE REGEX ".*\.(h|hpp|cuh)$")
source_group(TREE ${FLAMEGPU_ROOT}/src/flamegpu PREFIX src FILES ${T_SRC})
set(T_SRC "${SRC_EXTERNAL}")
list(FILTER T_SRC INCLUDE REGEX ".*\.(h|hpp|cuh)$")
source_group(TREE ${FLAMEGPU_ROOT}/include PREFIX external FILES ${T_SRC})
set(T_SRC "${SRC_EXTERNAL}")
list(FILTER T_SRC EXCLUDE REGEX ".*\.(h|hpp|cuh)$")
source_group(TREE ${FLAMEGPU_ROOT}/include PREFIX external FILES ${T_SRC})

# Define which source files are required for the target executable
add_flamegpu_library("${PROJECT_NAME}" "${ALL_SRC}" "${FLAMEGPU_ROOT}")

# Also set as startup project (if top level project)
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"  PROPERTY VS_STARTUP_PROJECT "${PROJECT_NAME}")
