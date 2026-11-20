cmake_minimum_required(VERSION 3.16)
project(cardsim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(SKETCH "" CACHE FILEPATH "The .ino sketch to run (its folder's other .ino/.cpp files come along)")
set(SKETCH_LIBS "" CACHE STRING "Extra Arduino library folders, separated by ';'")
option(CARDSIM_TINYGPS "Download and build TinyGPSPlus for the sketch" ON)
option(CARDSIM_WEB_JS_ONLY "Browser build: plain JavaScript instead of WebAssembly (for hosts whose security policy blocks WebAssembly)" OFF)

if(NOT SKETCH)
  message(FATAL_ERROR "Tell CardSim which sketch to run: -DSKETCH=path/to/sketch.ino")
endif()
get_filename_component(SKETCH "${SKETCH}" ABSOLUTE)
get_filename_component(SKETCH_NAME "${SKETCH}" NAME_WE)
get_filename_component(SKETCH_DIR "${SKETCH}" DIRECTORY)

# ---------------------------------------------------------------- the sketch
file(GLOB SKETCH_TABS "${SKETCH_DIR}/*.ino")
file(GLOB SKETCH_CPP "${SKETCH_DIR}/*.cpp" "${SKETCH_DIR}/*.c")
set(SKETCH_GEN "${CMAKE_BINARY_DIR}/sketch_${SKETCH_NAME}.cpp")
add_custom_command(
  OUTPUT "${SKETCH_GEN}"
  COMMAND ${CMAKE_COMMAND} "-DOUT=${SKETCH_GEN}" "-DMAIN=${SKETCH}" "-DTABS=${SKETCH_TABS}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ino2cpp.cmake"
  DEPENDS ${SKETCH_TABS} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ino2cpp.cmake"
  COMMENT "Preparing sketch ${SKETCH_NAME}")

# ---------------------------------------------------------------- libraries
set(LIB_SOURCES "")
set(LIB_INCLUDES "")
if(CARDSIM_TINYGPS)
  include(FetchContent)
  FetchContent_Declare(tinygpsplus
    GIT_REPOSITORY https://github.com/mikalhart/TinyGPSPlus.git
    GIT_TAG master
    GIT_SHALLOW TRUE)
  FetchContent_GetProperties(tinygpsplus)
  if(NOT tinygpsplus_POPULATED)
    FetchContent_Populate(tinygpsplus)
  endif()
  list(APPEND LIB_SOURCES "${tinygpsplus_SOURCE_DIR}/src/TinyGPS++.cpp")
  list(APPEND LIB_INCLUDES "${tinygpsplus_SOURCE_DIR}/src")
endif()
foreach(L ${SKETCH_LIBS})
  get_filename_component(L "${L}" ABSOLUTE)
  if(EXISTS "${L}/src")
    file(GLOB_RECURSE LSRC "${L}/src/*.cpp" "${L}/src/*.c")
    list(APPEND LIB_INCLUDES "${L}/src")
  else()
    file(GLOB LSRC "${L}/*.cpp" "${L}/*.c")
    list(APPEND LIB_INCLUDES "${L}")
  endif()
  list(APPEND LIB_SOURCES ${LSRC})
endforeach()

# ---------------------------------------------------------------- core
set(CORE_SOURCES
  core/src/sim.cpp
  core/src/world.cpp
  core/src/arduino.cpp
  core/src/gfx.cpp
  core/src/m5.cpp
  core/src/wifi.cpp
  core/src/fs.cpp)

set(ALL_SOURCES ${CORE_SOURCES} ${LIB_SOURCES} ${SKETCH_CPP} "${SKETCH_GEN}")
set(ALL_INCLUDES core/include ${LIB_INCLUDES} "${SKETCH_DIR}")

if(EMSCRIPTEN)
  # ---------------------------------------------------------------- browser
  add_executable(cardsim_web ${ALL_SOURCES} frontends/web/web_main.cpp)
  target_include_directories(cardsim_web PRIVATE ${ALL_INCLUDES})
  target_compile_definitions(cardsim_web PRIVATE CARDSIM=1 ARDUINO=10819 ESP32=1
    "CARDSIM_SKETCH_NAME=\"${SKETCH_NAME}\"")
  set_target_properties(cardsim_web PROPERTIES OUTPUT_NAME "${SKETCH_NAME}" SUFFIX ".html")
  target_link_options(cardsim_web PRIVATE
    -sASYNCIFY=1
    -sASYNCIFY_STACK_SIZE=65536
    -sALLOW_MEMORY_GROWTH=1
    -sFORCE_FILESYSTEM=1
    -sSINGLE_FILE=1
    -sMINIFY_HTML=0
    "-sEXPORTED_RUNTIME_METHODS=['FS','ccall','cwrap','UTF8ToString']"
    "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/frontends/web/shell.html")
  if(CARDSIM_WEB_JS_ONLY)
    target_link_options(cardsim_web PRIVATE -sWASM=0 -sDYNAMIC_EXECUTION=0)
  endif()
  set_property(TARGET cardsim_web APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/frontends/web/shell.html")
else()
  # ---------------------------------------------------------------- desktop
  find_package(SDL2 CONFIG QUIET)
  if(NOT SDL2_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
  endif()
  add_executable(cardsim_${SKETCH_NAME} ${ALL_SOURCES} frontends/native_main.cpp)
  target_include_directories(cardsim_${SKETCH_NAME} PRIVATE ${ALL_INCLUDES})
  target_compile_definitions(cardsim_${SKETCH_NAME} PRIVATE CARDSIM=1 ARDUINO=10819 ESP32=1)
  if(TARGET SDL2::SDL2main)
    target_link_libraries(cardsim_${SKETCH_NAME} PRIVATE SDL2::SDL2main)
  endif()
  target_link_libraries(cardsim_${SKETCH_NAME} PRIVATE SDL2::SDL2)
endif()
