project(VideoTracker)

cmake_minimum_required(VERSION 2.8)

set(RPI OFF)

if(RPI)
  set(CMAKE_MODULE_PATH ${})
  find_package(raspicam REQUIRED)
endif()

find_package(OpenCV REQUIRED)

if(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif()

# aux_source_directory(. SRC_LIST)
# add_executable(${PROJECT_NAME} ${SRC_LIST})
add_executable(CameraServer server.cpp tracker.cpp tracker2.cpp calibration.cpp Socket.cpp ServerSocket.cpp Snapshot.cpp)
if (RPI)
  target_link_libraries(CameraServer ${raspicam_CV_LIBS} -lpthread -lrt)
else()
  target_link_libraries(CameraServer ${OpenCV_LIBS} -lpthread -lrt)
endif()

add_executable(Client client.cpp Receiver.cpp Aggregator.cpp hungarian.cpp Socket.cpp Snapshot.cpp)
target_link_libraries(Client ${OpenCV_LIBS} -lpthread)
