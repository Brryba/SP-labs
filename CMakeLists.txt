cmake_minimum_required(VERSION 3.29)
project(SP)

set(CMAKE_CXX_STANDARD 26)

add_executable(SP main.cpp resource/resource.rc
        window/spriteWindow.cpp
        utils/timerUtils.h
        utils/timerUtils.cpp
        dialog/aboutDialog.cpp)
