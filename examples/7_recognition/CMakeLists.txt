# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/moonlight_board
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/led_rgb
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/speech_recognition
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/button
    )


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(moonlight)
