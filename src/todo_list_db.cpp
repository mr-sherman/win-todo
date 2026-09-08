/*
todo_list_db.hpp

This file implements the class for opening, managing, and modifying
the todo list sqlite database.

TODO:  A windows-native terminal application for managing todo lists.
Copyright 2025 by Bradford Sherman, @mr-sherman

No AI was used in the writing of this application.
I kick it old school.

*/

#include "todo_list_db.hpp"
#include <sstream>
#include <chrono>

namespace todo {

    int todo_list_db::create_list_entry(const std::string &item_text, const std::string &tag)
    {
        std::stringstream creation_time;
        int max_task = get_max_task_number() + 1;
        auto now = std::chrono::system_clock::now();
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm* local_tm = std::localtime(&time_t_now); // Or std::gmtime for UTC

        creation_time << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");

        sqlitepp::query q(_db);

        q <<    "INSERT INTO todolist (task_number, task_text, created_time, completed_time, tag) " <<
                "VALUES ( "<< max_task << ", ?, \""<< creation_time.str().c_str() <<"\", NULL, ?)";

        q.bind(1, item_text);
        q.bind(2, tag);

        auto result = q.exec();
        if (result != SQLITE_OK)
            throw todo_error(result, "Error inserting new task into database");
        return 0;
    }

    int todo_list_db::delete_list_entry(int task_number)
    {
        sqlitepp::query q(_db);
        //update the completed time field
        q <<    "DELETE FROM todolist WHERE task_number = " << 
                task_number;
        
        auto result = q.exec();

        if (result != SQLITE_OK)
            throw todo_error(result, "Error deleting task completed time in database");
        sqlitepp::query up(_db);

        up <<   "UPDATE todolist SET task_number = task_number - 1 WHERE task_number > " << task_number;
        result = up.exec();
        
        if (result != SQLITE_OK)
            throw todo_error(result, "Error in shifting task numbers in database");
        return result;
    
    }

    int todo_list_db::create_db()
    {
        std::stringstream  cmd_st;
        cmd_st  << "CREATE TABLE IF NOT EXISTS todolist"
                << "(task_number INT, "
                << "task_text TEXT, "
                << "created_time TEXT, "
                << "completed_time TEXT, "
                << "tag TEXT);"
        ;

        sqlitepp::query q(_db, cmd_st.str());
        int result = q.exec();

        if (result != SQLITE_OK)
            return result;

        // Databases created before the tag column existed won't have it yet;
        // add it here (no-op on a table just created by the statement above).
        sqlitepp::query info(_db, "PRAGMA table_info(todolist)");
        sqlitepp::result cols = info.store();
        bool has_tag_column = false;
        for (int i = 0; i < cols.num_rows(); ++i)
        {
            if (std::string(cols[i]["name"]) == "tag")
            {
                has_tag_column = true;
                break;
            }
        }
        if (!has_tag_column)
        {
            sqlitepp::query alter(_db, "ALTER TABLE todolist ADD COLUMN tag TEXT");
            result = alter.exec();
        }

        return result;
    }

    int todo_list_db::get_max_task_number()
    {
        int max_task = 0;

        std::string sel_cmd = "SELECT MAX(task_number) from todolist";
        sqlitepp::query q(_db, sel_cmd);

        sqlitepp::result res = q.store();
        if (res.num_rows() > 0)
            max_task = res[0]["MAX(task_number)"];
        
        return max_task;
        
    }

    todo_list todo_list_db::get_open_items(const std::string &tag_filter)
    {
        std::string sel_cmd = "SELECT rowid, task_number, task_text, created_time, tag FROM todolist WHERE task_number > 0";
        if (!tag_filter.empty())
            sel_cmd += " AND tag = ?";

        sqlitepp::query q(_db, sel_cmd);
        if (!tag_filter.empty())
            q.bind(1, tag_filter);
        sqlitepp::result res = q.store();

        todo_list list;
        list.reserve(res.num_rows());
        for (int i = 0 ; i < res.num_rows(); ++i)
        {
            // Rows written before the tag column existed store a SQL NULL
            // here; sqlitepp stringifies NULL as the literal text "NULL",
            // so that has to be turned into "no tag" explicitly.
            const auto& tag_field = res[i]["tag"];
            std::string tag = tag_field.is_null() ? std::string() : (std::string)tag_field;

            list.push_back(
                item_entry( (int)res[i]["task_number"],
                            res[i]["task_text"],
                            res[i]["created_time"],
                            (long long)res[i]["rowid"],
                            tag));
        }
        return list;
    }

    int todo_list_db::resolve_by_rowid(long long row_id)
    {
        sqlitepp::query sel(_db);
        sel << "SELECT task_number FROM todolist WHERE rowid = " << row_id;
        sqlitepp::result res = sel.store();

        if (res.num_rows() == 0)
            return 1; // no such row (already deleted, or never existed)

        int current_task_number = (int)res[0]["task_number"];
        if (current_task_number <= 0)
            return 0; // already resolved; nothing to do

        return resolve_list_entry(current_task_number);
    }

    int todo_list_db::resolve_list_entry(int task_number)
    {
        std::stringstream resolved_time;
        auto now = std::chrono::system_clock::now();
        std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm* local_tm = std::localtime(&time_t_now); // Or std::gmtime for UTC

        resolved_time << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");

        sqlitepp::query q(_db);
        //update the completed time field
        q <<    "UPDATE todolist SET completed_time = \"" << 
                resolved_time.str().c_str() << "\", task_number=0" << " WHERE task_number = " << 
                task_number;
        
        auto result = q.exec();

        if (result != SQLITE_OK)
            throw todo_error(result, "Error updating task completed time in database");
        // now we have to slide the task numbers down by 1
        sqlitepp::query up(_db);

        up <<   "UPDATE todolist SET task_number = task_number - 1 WHERE task_number > " << task_number;
        result = up.exec();
        
        if (result != SQLITE_OK)
            throw todo_error(result, "Error in shifting task numbers in database");
        return result;
    }

}