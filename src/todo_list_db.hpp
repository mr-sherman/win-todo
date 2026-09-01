/*
todo_list_db.hpp

This file defines the class for opening, managing, and modifying
the todo list sqlite database.

TODO:  A windows-native terminal application for managing todo lists.
Copyright 2025 by Bradford Sherman, @mr-sherman

No AI was used in the writing of this application.
I kick it old school.

*/

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "sqlitepp.hpp"
#include "todo_list.hpp"
#include <memory>


namespace todo {

    class todo_list_db
    {
    public:
        todo_list_db(const std::string &file_name) : _db(file_name){
            if (!_db.is_open())
            {
                throw todo_error(3, "Could not open database");
                ;
            }
        };

        int create_list_entry(const std::string &item_text);
        int delete_list_entry(int item_number);
        int resolve_list_entry(int item_number);

        int create_db();

        todo_list get_open_items();

    private:
        int get_max_task_number();

    private:
        sqlitepp::db _db;

    };
}