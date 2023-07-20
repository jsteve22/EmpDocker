cmake_minimum_required (VERSION 3.12)
project (emp-sh2pc)
set(NAME "emp-sh2pc")

# emp-sh2pc configuration
find_path(CMAKE_FOLDER NAMES cmake/emp-tool-config.cmake)
include(${CMAKE_FOLDER}/cmake/emp-base.cmake)

find_package(emp-ot REQUIRED)
include_directories(${EMP-OT_INCLUDE_DIRS})

# Installation
install(FILES cmake/emp-sh2pc-config.cmake DESTINATION cmake/)
install(DIRECTORY emp-sh2pc DESTINATION include/)

ENABLE_TESTING()

# gazelle configuration
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++17")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(LIB "src/lib")
set(BIN "src/bin")
add_library( DelphiOffline STATIC
    ${LIB}/interface.cpp ${LIB}/conv2d.cpp ${LIB}/fc_layer.cpp
    ${LIB}/triples_gen.cpp ${LIB}/read_txt.cpp ${LIB}/ReLU.cpp 
    ${LIB}/mean_pooling.cpp ${LIB}/flattens.cpp ${LIB}/dense_layer.cpp
    ${LIB}/conv_layer.cpp ${LIB}/extras.cpp ${LIB}/helper_functions.cpp
    ${LIB}/print_outputs.cpp ${LIB}/batchnorm_gc.cpp ${LIB}/max_pooling.cpp)
include_directories(${CMAKE_SOURCE_DIR}/src/lib)

include(FetchContent)
FetchContent_Declare(
  seal
  GIT_REPOSITORY https://github.com/microsoft/SEAL
  GIT_TAG        v3.6.5
)
option(SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT OFF)
option(SEAL_USE_ZLIB OFF)
option(SEAL_USE_ZSTD ON)
FetchContent_MakeAvailable(seal)

find_package(OpenMP REQUIRED)
target_link_libraries(DelphiOffline seal OpenMP::OpenMP_CXX)


option(UNITTESTS "Build unittests" OFF)
if(UNITTESTS)
    include_directories(${CMAKE_SOURCE_DIR}/src/bin)
    add_executable(unittests ${BIN}/unittests.cpp ${LIB}/im2col.cpp)
    add_executable(benchmark ${BIN}/benchmark.cpp ${LIB}/im2col.cpp)
    add_executable(interface ${BIN}/interface.c)

    # Unittests make use of Eigen
    find_package (Eigen3 3.3 REQUIRED NO_MODULE)
    target_link_libraries (unittests Eigen3::Eigen)
    target_link_libraries (benchmark Eigen3::Eigen)

    target_link_libraries (benchmark DelphiOffline)
    target_link_libraries (unittests DelphiOffline)
    target_link_libraries (interface DelphiOffline)
endif()

install(TARGETS DelphiOffline DESTINATION .)

ADD_SUBDIRECTORY(test)
