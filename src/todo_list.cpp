/*
todo_list.cpp

This file implements the functions defined in todo_list.hpp

TODO:  A windows-native terminal application for managing todo lists.
Copyright 2025 by Bradford Sherman, @mr-sherman

No AI was used in the writing of this application.
I kick it old school.

*/

#include "todo_list.hpp"
#include "todo_list_db.hpp"
#include <iomanip>
#include <iostream>

namespace todo
{
    std::ostream& operator<<(std::ostream& os, const item_entry& ie)
    {
        os << std::right << std::setw(6) << "  " << ie.item_number << "| " << std::left << std::setw(50) << ie.item_text;
        return os;
    }

    std::ostream& operator<<(std::ostream &os, const todo_list& l)
    {
        os << "Current open items:"<< std::endl;
        os << "-------------------"<< std::endl;
        os << "Item # | Task      "<< std::endl;
        os << "-------------------"<< std::endl;
        for (auto i : l)
        {
            os << i << std::endl;;
        }
        return os;
    }

}