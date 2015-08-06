cmake_minimum_required(VERSION 2.8.4)
project(nodetest)

set(EXEC_NAME nodetest)

find_package(ALSA) #alsa
find_package (Threads) #pthread
find_package(Boost 1.58.0) #boost

set(INCLUDE_DIR ./include)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS} ${INCLUDE_DIR} ${ALSA_INCLUDE_DIRS})

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  set(SOURCE_FILES
      main.cc
      src/wav_functions.cc
      src/alsa_control.cc
  )

  add_executable(${EXEC_NAME} ${SOURCE_FILES})

  target_link_libraries(${EXEC_NAME} ${CMAKE_THREAD_LIBS_INIT} ${ALSA_LIBRARIES} ${Boost_LIBRARIES} rt)

endif()