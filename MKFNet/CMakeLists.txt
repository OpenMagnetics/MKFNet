cmake_minimum_required(VERSION 3.16)

project(MKFWrapperProj VERSION 1.0 LANGUAGES CXX)

option(BUILD_TESTS      "Build tests"    OFF)
option(BUILD_EXAMPLES   "Build examples" OFF)
option(UTPP_INCLUDE_TESTS_IN_BUILD   "Build tests" OFF)
option(INCLUDE_MKF_TESTS      "Build tests"    OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EMBED_MAS_DATA ON)
set(EMBED_MAS_CORES ON)
set(EMBED_MAS_CORE_SHAPES ON)
set(EMBED_MAS_CORE_MATERIALS ON)
set(EMBED_MAS_WIRES ON)
set(EMBED_MAS_WIRE_MATERIALS ON)
set(EMBED_MAS_BOBBINS ON)
set(EMBED_MAS_INSULATION_MATERIALS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else ()
    if (WIN32)
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W")
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -allow-multiple-definition")
        # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-linker-plugin")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -Wno-switch")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
    endif()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    # set(CMAKE_BUILD_TYPE RelWithDebInfo)
    # set(CMAKE_BUILD_TYPE MinSizeRel)
    set(CMAKE_BUILD_TYPE Release)
endif()

SET(MAS_DIRECTORY "${CMAKE_BINARY_DIR}/MAS/")
SET(MAS_DIR "${CMAKE_BINARY_DIR}/_deps/mas-src/")
SET(MKF_DIR "${CMAKE_BINARY_DIR}/_deps/mkf-src/")
message(STATUS MAS_DIRECTORY)
message(STATUS ${MAS_DIRECTORY})
message(STATUS MAS_DIR)
message(STATUS ${MAS_DIR})
message(STATUS MKF_DIR)
message(STATUS ${MKF_DIR})

include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json/
    GIT_TAG  tags/v3.11.2)
FetchContent_MakeAvailable(json)
include_directories("${CMAKE_BINARY_DIR}/_deps/json-src/include/nlohmann/")
include_directories("${CMAKE_BINARY_DIR}/_deps/json-src/include/")

FetchContent_Declare(spline
    GIT_REPOSITORY https://github.com/AlfVII/spline.git)
FetchContent_MakeAvailable(spline)
include_directories("${CMAKE_BINARY_DIR}/_deps/spline-src/src")

FetchContent_Declare(magic-enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum
    GIT_TAG  tags/v0.9.3)
FetchContent_MakeAvailable(magic-enum)
include_directories("${CMAKE_BINARY_DIR}/_deps/magic-enum-src/include")

FetchContent_Declare(matplotplusplus
    GIT_REPOSITORY https://github.com/alandefreitas/matplotplusplus
    GIT_TAG origin/master) # or whatever tag you want
FetchContent_GetProperties(matplotplusplus)
if(NOT matplotplusplus_POPULATED)
    FetchContent_Populate(matplotplusplus)
    add_subdirectory(${matplotplusplus_SOURCE_DIR} ${matplotplusplus_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(MKF
        GIT_REPOSITORY https://github.com/OpenMagnetics/MKF.git
        GIT_TAG 0916cd794c4faf70bda92c34aa43ae402a27b1a1)

FetchContent_Declare(magic-enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum
        GIT_TAG  tags/v0.9.3)
FetchContent_MakeAvailable(magic-enum)

FetchContent_Declare(
       mas
       GIT_REPOSITORY https://github.com/OpenMagnetics/MAS.git
       GIT_TAG 617b4fa64fecda35be5f40cfa831296827570aff
)
FetchContent_GetProperties(mas)
FetchContent_GetProperties(MKF)
message(STATUS ${MAS_POPULATED})
if(NOT MAS_POPULATED)
  FetchContent_Populate(mas)
endif()
message(STATUS ${MKF_POPULATED})
if(NOT MKF_POPULATED)
  FetchContent_Populate(MKF)
endif()
message(STATUS ${MAS_SOURCE_DIR})

add_custom_command(
  OUTPUT "${MAS_DIRECTORY}/MAS.hpp"
  COMMAND quicktype -l c++ -s schema ${MAS_DIR}/schemas/MAS.json -S ${MAS_DIR}/schemas/magnetic.json -S ${MAS_DIR}/schemas/magnetic/core.json -S ${MAS_DIR}/schemas/magnetic/coil.json -S ${MAS_DIR}/schemas/utils.json -S ${MAS_DIR}/schemas/magnetic/core/gap.json -S ${MAS_DIR}/schemas/magnetic/core/shape.json -S ${MAS_DIR}/schemas/magnetic/core/material.json -S ${MAS_DIR}/schemas/magnetic/insulation/material.json -S ${MAS_DIR}/schemas/magnetic/insulation/wireCoating.json -S ${MAS_DIR}/schemas/magnetic/bobbin.json -S ${MAS_DIR}/schemas/magnetic/core/piece.json -S ${MAS_DIR}/schemas/magnetic/core/spacer.json -S ${MAS_DIR}/schemas/magnetic/wire/basicWire.json -S ${MAS_DIR}/schemas/magnetic/wire/round.json -S ${MAS_DIR}/schemas/magnetic/wire/rectangular.json -S ${MAS_DIR}/schemas/magnetic/wire/foil.json -S ${MAS_DIR}/schemas/magnetic/wire/planar.json -S ${MAS_DIR}/schemas/magnetic/wire/litz.json -S ${MAS_DIR}/schemas/magnetic/wire/material.json -S ${MAS_DIR}/schemas/magnetic/wire.json -S ${MAS_DIR}/schemas/utils.json -S ${MAS_DIR}/schemas/magnetic/insulation/wireCoating.json -S ${MAS_DIR}/schemas/magnetic/insulation/material.json -S ${MAS_DIR}/schemas/inputs.json -S ${MAS_DIR}/schemas/outputs.json -S ${MAS_DIR}/schemas/outputs/coreLossesOutput.json -S ${MAS_DIR}/schemas/inputs/designRequirements.json -S ${MAS_DIR}/schemas/inputs/operatingPoint.json -S ${MAS_DIR}/schemas/inputs/operatingConditions.json -S ${MAS_DIR}/schemas/inputs/operatingPointExcitation.json -o ${MAS_DIRECTORY}/MAS.hpp --namespace OpenMagnetics --source-style single-source --type-style pascal-case --member-style underscore-case --enumerator-style upper-underscore-case --no-boost
  USES_TERMINAL)

add_custom_target(MASNetGeneration
                  DEPENDS "${MAS_DIRECTORY}/MAS.hpp")

file(GLOB SOURCES MKFNet.i MKFNet.cpp ${CMAKE_BINARY_DIR}/_deps/mkf-src/src/*.cpp)




find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})



# set(CMAKE_BUILD_TYPE Release)

# Set .Net project directory
# set(NET_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../MKFNetTest")
# set(NET_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../MKFNetTest")
if ("${NET_PROJECT_DIR}" STREQUAL "")
    set(NET_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../MKFNetTest")
    # set(NET_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../MMM/src/MMM/Windows")
endif()
message("NET_PROJECT_DIR: ${NET_PROJECT_DIR}")

# Add swig flags here
set(CMAKE_SWIG_FLAGS "")

set_property(SOURCE MKFNet.i PROPERTY CPLUSPLUS ON)
set_source_files_properties(MKFNet.i PROPERTIES SWIG_FLAGS "-includeall")

file(REMOVE [${NET_PROJECT_DIR}/MKFNet.cs ${NET_PROJECT_DIR}/MKFNetCSHARP_wrap.cxx ${NET_PROJECT_DIR}/MKFNetModule.cs ${NET_PROJECT_DIR}/MKFNetModulePINVOKE.cs])

swig_add_library(MKFNet
  TYPE SHARED
  LANGUAGE CSharp
  SOURCES ${SOURCES}
  OUTPUT_DIR ${NET_PROJECT_DIR}
  OUTFILE_DIR ${NET_PROJECT_DIR}
  )


# for copying MKFNet.dll to .Net project dir dir
set_target_properties( MKFNet
    PROPERTIES
    # These copy MKFNet.dll on Windows to .Net project directory
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${NET_PROJECT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${NET_PROJECT_DIR}
    
    # This copies MKFNet.so on Linux to .Net project directory
    LIBRARY_OUTPUT_DIRECTORY ${NET_PROJECT_DIR}

    # Set address of C++ headers
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
)





target_link_libraries(MKFNet PUBLIC nlohmann_json::nlohmann_json matplot)

add_dependencies(MKFNet MASNetGeneration)


file(DOWNLOAD "https://raw.githubusercontent.com/vector-of-bool/cmrc/master/CMakeRC.cmake"
                 "${CMAKE_BINARY_DIR}/CMakeRC.cmake")
include("${CMAKE_BINARY_DIR}/CMakeRC.cmake")

include_directories("${CMAKE_BINARY_DIR}/_deps/mkf-src/")

cmrc_add_resource_library(insulation_standards ALIAS data::insulation_standards NAMESPACE insulationData WHENCE ${MKF_DIR}/ ${MKF_DIR}/src/data/insulation_standards/IEC_60664-1.json ${MKF_DIR}/src/data/insulation_standards/IEC_60664-4.json ${MKF_DIR}/src/data/insulation_standards/IEC_60664-5.json ${MKF_DIR}/src/data/insulation_standards/IEC_62368-1.json ${MKF_DIR}/src/data/insulation_standards/IEC_61558-1.json ${MKF_DIR}/src/data/insulation_standards/IEC_61558-2-16.json ${MKF_DIR}/src/data/insulation_standards/IEC_60335-1.json)
target_link_libraries(MKFNet PUBLIC data::insulation_standards)


cmrc_add_resource_library(data ALIAS data::data NAMESPACE data WHENCE ${MAS_DIR} PREFIX MAS ${MAS_DIR}/data/core_materials.ndjson ${MAS_DIR}/data/core_shapes.ndjson ${MAS_DIR}/data/cores.ndjson ${MAS_DIR}/data/bobbins.ndjson ${MAS_DIR}/data/insulation_materials.ndjson ${MAS_DIR}/data/wire_materials.ndjson ${MAS_DIR}/data/wires.ndjson)
target_link_libraries(MKFNet PUBLIC data::data)


include_directories("${CMAKE_BINARY_DIR}/_deps/json-src/include/nlohmann/")
include_directories("${CMAKE_BINARY_DIR}/_deps/pybind11-src/include/")
include_directories("${CMAKE_BINARY_DIR}/_deps/pybind11_json-src/include/pybind11_json/")
include_directories("${CMAKE_BINARY_DIR}/_deps/json-src/include/nlohmann/")
include_directories("${CMAKE_BINARY_DIR}/_deps/magic-enum-src/include")
include_directories("${CMAKE_BINARY_DIR}/_deps/svg-src/src")
include_directories("${CMAKE_BINARY_DIR}/_deps/spline-src/src")
include_directories("${CMAKE_BINARY_DIR}/_deps/json-src/include/")
include_directories("${CMAKE_BINARY_DIR}/_deps/mkf-src/src/")
include_directories("${CMAKE_BINARY_DIR}/_cmrc/include")
include_directories("${MAS_DIRECTORY}")
