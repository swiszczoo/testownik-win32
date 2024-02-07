cmake_minimum_required(VERSION 3.20)
project(testownik_win32 VERSION 1.0.0)

file(GLOB_RECURSE SOURCES "src/*.*")

add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    win32/testownik.manifest
    win32/testownik.rc
)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DUNICODE -D_UNICODE)

# Disable CRT to decrease executable size
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE /GS- /Os)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DNOCRT)
    target_link_options(${PROJECT_NAME} PRIVATE /NODEFAULTLIB /ENTRY:wWinMain)
    target_link_directories(${PROJECT_NAME} PRIVATE crt)
    target_link_libraries(${PROJECT_NAME} PRIVATE msvcrt-light-x64)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE comctl32 pathcch shlwapi)
target_include_directories(${PROJECT_NAME} PRIVATE include)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Testownik")
