cmake_minimum_required(VERSION 3.16)
project(DemonSlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Source files ──────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/Entity.cpp
    src/Player.cpp
    src/Obstacle.cpp
    src/Game.cpp
    src/Menu.cpp
)

add_executable(DemonSlayer ${SOURCES})

target_include_directories(DemonSlayer PRIVATE include)

# ── Find SFML (Graphics + Audio) ─────────────────────────────────────────────
find_package(SFML 2.6 COMPONENTS graphics window system audio QUIET)

if(SFML_FOUND)
    target_link_libraries(DemonSlayer PRIVATE
        sfml-graphics sfml-window sfml-system sfml-audio)
else()
    message(WARNING "SFML not found via find_package. "
            "Set -DSFML_DIR=<path/to/SFML> or install SFML 2.6.")
    target_link_libraries(DemonSlayer PRIVATE
        sfml-graphics sfml-window sfml-system sfml-audio)
endif()

# ── Copy assets next to executable ───────────────────────────────────────────
add_custom_command(TARGET DemonSlayer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/asseste
        $<TARGET_FILE_DIR:DemonSlayer>/asseste
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Montserrat
        $<TARGET_FILE_DIR:DemonSlayer>/Montserrat
    COMMENT "Copying assets..."
)

# ── Windows: copy SFML DLLs automatically if available ───────────────────────
if(WIN32 AND SFML_FOUND)
    add_custom_command(TARGET DemonSlayer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:DemonSlayer>
            $<TARGET_FILE_DIR:DemonSlayer>
        COMMAND_EXPAND_LISTS
    )
endif()
