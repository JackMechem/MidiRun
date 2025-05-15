cmake_minimum_required(VERSION 3.0)
project(midirun VERSION 1.2.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

if(APPLE)

  set(CMAKE_EXE_LINKER_FLAGS "-framework CoreFoundation -framework Carbon")

endif(APPLE)

# find_package(RtMidi) add_subdirectory(lib/rtmidi)

include(FetchContent)

FetchContent_Declare(
  tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG v3.4.0)
FetchContent_MakeAvailable(tomlplusplus)

FetchContent_Declare(
  rtmidi_src
  GIT_REPOSITORY https://github.com/thestk/rtmidi.git
  GIT_TAG 6.0.0)
FetchContent_MakeAvailable(rtmidi_src)

add_library(rtmidi::rtmidi INTERFACE IMPORTED)
target_link_libraries(rtmidi::rtmidi INTERFACE rtmidi)
target_include_directories(rtmidi::rtmidi INTERFACE ${rtmidi_src_SOURCE_DIR})

add_executable(midirun src/main.cpp)

target_include_directories(midirun PUBLIC src)

target_link_libraries(midirun rtmidi::rtmidi)
target_link_libraries(midirun tomlplusplus::tomlplusplus)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

  install(TARGETS ${PROJECT_NAME} DESTINATION bin)

endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(APPLE)

  install(TARGETS ${PROJECT_NAME} DESTINATION bin)

endif(APPLE)
