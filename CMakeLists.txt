cmake_minimum_required(VERSION 3.16)

project(todo
    VERSION 1.0.0
    DESCRIPTION "A terminal to-do list application backed by SQLite"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(todo
    src/todo.cpp
    src/todo_list_db.cpp
    src/todo_list.cpp
)

target_link_libraries(todo PRIVATE
    unofficial::sqlite3::sqlite3
    Boost::program_options
)

if(WIN32)
    target_compile_definitions(todo PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

install(TARGETS todo RUNTIME DESTINATION bin)
