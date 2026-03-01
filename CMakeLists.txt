cmake_minimum_required(VERSION 3.10)
project(CrosshairOverlay)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(CrosshairOverlay WIN32 overlay.cpp icon.rc)
target_link_libraries(CrosshairOverlay d2d1 dwmapi shell32 comdlg32)

add_definitions(-DUNICODE -D_UNICODE)
