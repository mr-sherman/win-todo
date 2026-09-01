/*
todo_list.hpp

This file defines the items that make up a todo list.

TODO:  A windows-native terminal application for managing todo lists.
Copyright 2025 by Bradford Sherman, @mr-sherman

No AI was used in the writing of this application.
I kick it old school.

*/

#pragma once
#include <string>
#include <chrono>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
namespace todo
{
    class todo_error
    {
        public:
        todo_error(int error_code, std::string error_string): _e(error_code), _str(error_string)
        {}; 

        friend std::ostream& operator<<(std::ostream& os, const todo_error& ie)
        {
            os  << "Todo Error: "<< std::endl << "Error code: " 
                << ie._e << std::endl << "Error Message: " << ie._str << std::endl;
            return os;
        };

        int error() const { return _e;} 
        private:
        int _e;
        std::string _str;
    };

    const int TASK_STR_WIDTH = 1;

    class item_entry
    {
    public:
        item_entry(std::string t, int tn, 
            std::chrono::time_point<std::chrono::system_clock> ct, 
            std::chrono::time_point<std::chrono::system_clock> rt) :
            item_text(t), item_number(tn), creation_time(ct), resolved_time(rt), 
            is_resolved(true) {};

            item_entry(std::string t, int tn, 
            std::chrono::time_point<std::chrono::system_clock> ct) :
            item_text(t), item_number(tn), creation_time(ct), 
            is_resolved(false) {};
            
        item_entry(int tn, std::string t, std::string ts, long long rid) :
            item_text(t), item_number(tn), is_resolved(false), row_id(rid)
        {

            std::tm tm = {}; // Initialize to all zeros
            std::istringstream ss(ts);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

            auto to_time_t = std::mktime(&tm);

            creation_time = std::chrono::system_clock::from_time_t(to_time_t);

        };
        friend std::ostream& operator<<(std::ostream& os, const item_entry& ie);
        friend std::string to_markdown_checklist_line(const item_entry& ie);

    private:
        std::string item_text;
        int item_number;
        std::chrono::time_point<std::chrono::system_clock> creation_time;
        std::chrono::time_point<std::chrono::system_clock> resolved_time;
        bool is_resolved;
        long long row_id = 0; // SQLite rowid; stable across renumbering, unlike item_number
    };

    typedef std::vector<item_entry> todo_list;
    std::ostream& operator<<(std::ostream &os, const todo_list& l);

    // Formats a single item as a GitHub-style markdown checklist line, e.g.
    // "- [ ] buy milk <!-- id:3 -->". The trailing id comment is how
    // `todo import` maps a checked box back to the right database row.
    std::string to_markdown_checklist_line(const item_entry& ie);

    // Formats the full "## Things to do for <date>" export document for a
    // list of open items.
    std::string to_markdown_export(const todo_list& l, const std::string& date_str);
}