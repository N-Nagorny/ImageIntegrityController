cmake_minimum_required(VERSION 3.5)

project(koch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

set(SOURCES
    src/koch.cpp
)

set(HEADERS
    include/
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${HEADERS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
)
