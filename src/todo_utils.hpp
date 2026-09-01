/*
todo_utils.hpp

This file is for free-standing functions and other helpers

TODO:  A windows-native terminal application for managing todo lists.
Copyright 2025 by Bradford Sherman, @mr-sherman

No AI was used in the writing of this application.
I kick it old school.

*/

#pragma once

#include <string>
#include <algorithm> // For std::transform
#include <cctype>    // For ::toupper
#include <ctime>
#include <iomanip>
#include <sstream>

namespace todo
{
    inline std::string to_upper(const std::string &s)
    {
        std::string up_str;
        up_str.assign(s);
        std::transform(s.begin(), s.end(), up_str.begin(), ::toupper);

        return up_str;
    }

    inline std::string current_date_string()
    {
        std::time_t t = std::time(nullptr);
        std::tm* local_tm = std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(local_tm, "%Y-%m-%d");
        return oss.str();
    }
}