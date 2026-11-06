cmake_minimum_required(VERSION 3.12)

project(toggle_tree LANGUAGES CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

add_library(toggle_tree INTERFACE)
add_library(toggle_tree::toggle_tree ALIAS toggle_tree)

target_compile_features(toggle_tree INTERFACE
    cxx_std_20
)

target_include_directories(toggle_tree INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(toggle_tree INTERFACE
    Threads::Threads
)

option(TOGGLE_TREE_USE_PARLAY
    "Use Parlay's parallel runtime instead of Toggle Tree's scheduler"
    OFF
)

if(TOGGLE_TREE_USE_PARLAY)
    if(TARGET Parlay::parlay)
        set(_toggle_tree_parlay_target Parlay::parlay)
    elseif(TARGET parlay)
        set(_toggle_tree_parlay_target parlay)
    else()
        find_package(Parlay CONFIG REQUIRED)
        set(_toggle_tree_parlay_target Parlay::parlay)
    endif()
    target_compile_definitions(toggle_tree INTERFACE
        TOGGLE_TREE_USE_PARLAY
    )
    target_link_libraries(toggle_tree INTERFACE
        ${_toggle_tree_parlay_target}
    )
endif()

if(NOT CMAKE_CROSSCOMPILING AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(toggle_tree INTERFACE
        "$<BUILD_INTERFACE:$<$<NOT:$<CONFIG:Debug>>:-march=native>>"
    )
endif()
