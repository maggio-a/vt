project(VideoTracker)

cmake_minimum_required(VERSION 2.8)

if (NOT DEFINED RPI)
  set(RPI ON)
endif()

if(RPI)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/local/lib/cmake")
  find_package(raspicam REQUIRED)
endif()

find_package(OpenCV REQUIRED)

if(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++0x")
endif()

# aux_source_directory(. SRC_LIST)
# add_executable(${PROJECT_NAME} ${SRC_LIST})
add_executable(CameraServer server.cpp tracker.cpp tracker2.cpp calibration.cpp Socket.cpp ServerSocket.cpp Snapshot.cpp)
if (RPI)
  target_link_libraries(CameraServer ${raspicam_CV_LIBS} pthread rt)
else()
  target_link_libraries(CameraServer ${OpenCV_LIBS} pthread rt)
endif()

add_executable(Client client.cpp Receiver.cpp Aggregator.cpp hungarian.cpp Socket.cpp Snapshot.cpp)
target_link_libraries(Client ${OpenCV_LIBS} pthread rt)
