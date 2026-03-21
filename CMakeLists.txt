cmake_minimum_required(VERSION 3.16)
project(cabin_drowsiness_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc objdetect highgui videoio)

add_executable(cabin_drowsiness_demo main.cpp)
target_link_libraries(cabin_drowsiness_demo PRIVATE ${OpenCV_LIBS})