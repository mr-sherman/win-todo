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

        int create_list_entry(const std::string &item_text, const std::string &tag = "");
        int delete_list_entry(int item_number);
        int resolve_list_entry(int item_number);

        // Resolves whatever item currently occupies this SQLite rowid, if
        // it's still open. Unlike task_number, rowid never shifts, so this
        // is the safe way to close an item captured by an earlier export
        // (see todo_list::to_markdown_checklist_line). A no-op if the row
        // doesn't exist or is already resolved.
        int resolve_by_rowid(long long row_id);

        int create_db();

        // With an empty tag_filter, returns all open items. Otherwise
        // returns only items whose tag matches tag_filter exactly.
        todo_list get_open_items(const std::string &tag_filter = "");

        // Same as get_open_items, but also includes completed items (for
        // 'list --all'). With an empty tag_filter, returns everything.
        todo_list get_all_items(const std::string &tag_filter = "");

    private:
        int get_max_task_number();

    private:
        sqlitepp::db _db;

    };
}