cmake_minimum_required(VERSION 3.15)
project(SupernautCpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-DNOMINMAX)
endif()

# Emit runtime binaries exactly where requested.
set(SUPERNAUT_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${SUPERNAUT_BIN_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${SUPERNAUT_BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${SUPERNAUT_BIN_DIR}")
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${cfg}" cfg_u)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg_u} "${SUPERNAUT_BIN_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg_u} "${SUPERNAUT_BIN_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg_u} "${SUPERNAUT_BIN_DIR}")
endforeach()

add_definitions(-DCPPHTTPLIB_NO_OPENSSL)

add_library(asxr_core STATIC
    runtime_fallback/runtime.cpp
)
target_include_directories(asxr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_fallback
)

add_executable(skill_factory_engine
    sf_main.cpp
)
target_link_libraries(skill_factory_engine PRIVATE asxr_core)

add_executable(supernaut_native
    main.cpp
    src/supernaut_orchestrator.cpp
)
target_include_directories(supernaut_native PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(supernaut_native PRIVATE ws2_32)

file(MAKE_DIRECTORY "${SUPERNAUT_BIN_DIR}/programs")
file(MAKE_DIRECTORY "${SUPERNAUT_BIN_DIR}/sco")
file(MAKE_DIRECTORY "${SUPERNAUT_BIN_DIR}/schemas")
file(MAKE_DIRECTORY "${SUPERNAUT_BIN_DIR}/micronauts")
file(MAKE_DIRECTORY "${SUPERNAUT_BIN_DIR}/maps")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/manifest.json"
               "${SUPERNAUT_BIN_DIR}/manifest.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/programs/main.json"
               "${SUPERNAUT_BIN_DIR}/programs/main.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/programs/stdlib.json"
               "${SUPERNAUT_BIN_DIR}/programs/stdlib.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/sco/registry.json"
               "${SUPERNAUT_BIN_DIR}/sco/registry.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/schemas/semantic_genome.jsonl"
               "${SUPERNAUT_BIN_DIR}/schemas/semantic_genome.jsonl" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/schemas/system_topology.xml"
               "${SUPERNAUT_BIN_DIR}/schemas/system_topology.xml" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/schemas/micronaut_ecology.xml"
               "${SUPERNAUT_BIN_DIR}/schemas/micronaut_ecology.xml" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/micronauts/scheduling_assistant.json"
               "${SUPERNAUT_BIN_DIR}/micronauts/scheduling_assistant.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/micronauts/career_coach.json"
               "${SUPERNAUT_BIN_DIR}/micronauts/career_coach.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/micronauts/manifest_architect.json"
               "${SUPERNAUT_BIN_DIR}/micronauts/manifest_architect.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/maps/tools.map.json"
               "${SUPERNAUT_BIN_DIR}/maps/tools.map.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/maps/micronauts.map.json"
               "${SUPERNAUT_BIN_DIR}/maps/micronauts.map.json" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/runtime_assets/maps/commands.map.json"
               "${SUPERNAUT_BIN_DIR}/maps/commands.map.json" COPYONLY)
