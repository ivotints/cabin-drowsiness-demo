cmake_minimum_required(VERSION 3.16)
project(cabin_drowsiness_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc objdetect highgui videoio)

add_library(drowsiness_monitor drowsiness_monitor.cpp)
add_library(alert_sound alert_sound.cpp)
add_library(cascade_loader cascade_loader.cpp)

add_executable(cabin_drowsiness_demo main.cpp)
target_link_libraries(cabin_drowsiness_demo PRIVATE drowsiness_monitor alert_sound cascade_loader ${OpenCV_LIBS})
