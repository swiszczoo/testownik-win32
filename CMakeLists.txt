cmake_minimum_required(VERSION 3.20)
project(testownik_win32 VERSION 1.0.0)

set(CMAKE_C_STANDARD 17)

file(GLOB_RECURSE SOURCES "src/*.*")
set(WIN_VERSION "${CMAKE_PROJECT_VERSION_MAJOR},${CMAKE_PROJECT_VERSION_MINOR},${CMAKE_PROJECT_VERSION_PATCH},0")

add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    win32/testownik.manifest
    win32/testownik.rc
)

target_compile_definitions(${PROJECT_NAME} PRIVATE -DUNICODE -D_UNICODE)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DRC_VERSION=${WIN_VERSION})
target_compile_definitions(${PROJECT_NAME} PRIVATE -DVERSION_STRING="${CMAKE_PROJECT_VERSION}")

# Link against built-in CRT (MSVCRT.dll) to remove all runtime dependencies
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE /GS- /Os)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DNOCRT -DUSE_MSVCRT)
    target_link_options(${PROJECT_NAME} PRIVATE /NODEFAULTLIB /ENTRY:wWinMain)
    target_link_directories(${PROJECT_NAME} PRIVATE crt)
    target_link_libraries(${PROJECT_NAME} PRIVATE msvcrt-light-x64)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    comctl32
    shlwapi
    ole32
    shell32
    windowscodecs
    msimg32
)
target_include_directories(${PROJECT_NAME} PRIVATE include win32)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Testownik")
