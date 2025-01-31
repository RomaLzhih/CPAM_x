cmake_minimum_required(VERSION 3.15)

project(
    cpam
    VERSION 1.0
    DESCRIPTION "KD tree"
    HOMEPAGE_URL ""
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

option(DEBUG "" ON)
option(CILKPLUS "" OFF)
option(OPENCILK "" OFF)
option(SERIAL "" OFF)
option(STDALLOC "" OFF)
option(CGAL "" ON)
option(MEMCHECK "" OFF)
option(JEMA "" OFF)

if(CILKPLUS)
    add_compile_options(-DPARLAY_CILKPLUS -DCILK -fcilkplus)
elseif(OPENCILK)
    add_compile_options(-DPARLAY_OPENCILK -DCILK -fopencilk)
elseif(SERIAL)
    add_compile_options(-DPARLAY_SEQUENTIAL)
else()
    add_compile_options(-pthread)
endif()

if(DEBUG)
    set(CMAKE_BUILD_TYPE "Debug")
    add_compile_options(-O3 -mcx16 -march=native)
elseif(MEMCHECK)
    set(CMAKE_BUILD_TYPE "Debug")
    add_compile_options(-Og -mcx16 -DPARLAY_SEQUENTIAL)
else()
    set(CMAKE_BUILD_TYPE "Release")
    add_compile_options(-O3 -mcx16 -march=native)
endif()

if(STDALLOC)
    add_compile_options(-DPARLAY_USE_STD_ALLOC)
elseif(JEMA)
    include_directories("/usr/local/lib")
    set(CMAKE_BUILD_RPATH "/usr/local/lib")
endif()

add_subdirectory(parlaylib)

# add_library(pam INTERFACE)
# set(PAM_INCLUDE_DIR "include")
# target_include_directories(pam INTERFACE ${PAM_INCLUDE_DIR})
# target_link_libraries(pam INTERFACE parlay)
# add_library(cpam INTERFACE)
# set(CPAM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
# target_include_directories(cpam INTERFACE ${CPAM_INCLUDE_DIR})
# target_link_libraries(cpam INTERFACE parlay)

# add_subdirectory(examples)
add_executable(cpam_query "../examples/range_query/rt_test.cpp")
target_include_directories(cpam_query PUBLIC "include" "examples/range_query/" "common")
target_link_libraries(cpam_query PUBLIC parlay)

# target_link_libraries(cpam_query)
if(JEMA)
    target_link_libraries(cpam_query PUBLIC libjemalloc.so)
endif()

FIND_PROGRAM(CLANG_FORMAT "clang-format")

IF(CLANG_FORMAT)
    add_custom_target(
        format
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${CLANG_FORMAT}
        -style=file
        -i
        ${all_files}
    )
endif()