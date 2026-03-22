# AUTOSAR Compliant CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(cabin_drowsiness_demo LANGUAGES CXX)

# AUTOSAR requires C++14 or higher, using C++17 for better type safety
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# AUTOSAR compliance: strict warning levels and error handling
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(AUTOSAR_WARNING_FLAGS
        -Wall
        -Wextra
        -Werror
        -Wpedantic
        -Wcast-align
        -Wcast-qual
        -Wconversion
        -Wctor-dtor-privacy
        -Wdisabled-optimization
        -Wformat=2
        -Winit-self
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wold-style-cast
        -Woverloaded-virtual
        -Wredundant-decls
        -Wshadow
        -Wsign-conversion
        -Wsign-promo
        -Wstrict-overflow=5
        -Wswitch-default
        -Wundef
        -Wno-unused
        -Wno-variadic-macros
        -Wno-parentheses
        -fstrict-aliasing
        -Wstrict-aliasing
    )
endif()

# Security flags for AUTOSAR compliance
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND AUTOSAR_WARNING_FLAGS
        -fstack-protector-strong
        -fPIE
        -fPIC
    )

    # AUTOSAR: Only enable _FORTIFY_SOURCE with optimization
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        list(APPEND AUTOSAR_WARNING_FLAGS -D_FORTIFY_SOURCE=2)
    endif()
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc objdetect highgui videoio)

# AUTOSAR compliant libraries with proper interface specifications
add_library(drowsiness_monitor drowsiness_monitor.cpp)
target_compile_features(drowsiness_monitor PUBLIC cxx_std_17)
target_compile_options(drowsiness_monitor PRIVATE ${AUTOSAR_WARNING_FLAGS})

add_library(alert_sound alert_sound.cpp)
target_compile_features(alert_sound PUBLIC cxx_std_17)
target_compile_options(alert_sound PRIVATE ${AUTOSAR_WARNING_FLAGS})

add_library(cascade_loader cascade_loader.cpp)
target_compile_features(cascade_loader PUBLIC cxx_std_17)
target_compile_options(cascade_loader PRIVATE ${AUTOSAR_WARNING_FLAGS})

# Main executable with AUTOSAR compliance flags
add_executable(cabin_drowsiness_demo main.cpp)
target_compile_features(cabin_drowsiness_demo PRIVATE cxx_std_17)
target_compile_options(cabin_drowsiness_demo PRIVATE ${AUTOSAR_WARNING_FLAGS})
target_link_libraries(cabin_drowsiness_demo PRIVATE drowsiness_monitor alert_sound cascade_loader ${OpenCV_LIBS})

# AUTOSAR: Enable static analysis if available
option(ENABLE_STATIC_ANALYSIS "Enable static analysis tools" OFF)
if(ENABLE_STATIC_ANALYSIS)
    find_program(CPPCHECK cppcheck)
    if(CPPCHECK)
        set(CMAKE_CXX_CPPCHECK ${CPPCHECK}
            --enable=warning,performance,portability,information,missingInclude
            --std=c++17
            --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
            --verbose
            --quiet)
    endif()
endif()
