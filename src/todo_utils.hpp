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
#include <chrono>
#include <format>
#include <optional>
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

    // Local wall-clock "now", truncated to whole seconds.
    //
    // These use std::chrono's time zone database instead of std::localtime.
    // std::localtime hands back a pointer to a single shared static std::tm,
    // so two threads calling it race, and any later call clobbers an earlier
    // caller's result. std::chrono::zoned_time returns a value instead.
    //
    // Truncating to seconds matters: system_clock::now() carries sub-second
    // precision, and formatting that with %S would emit fractional seconds
    // ("14:23:05.1234567"), which is not the format stored in the database.
    inline std::chrono::zoned_time<std::chrono::seconds> local_now()
    {
        return std::chrono::zoned_time{
            std::chrono::current_zone(),
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())};
    }

    inline std::string current_date_string()
    {
        return std::format("{:%Y-%m-%d}", local_now());
    }

    // "YYYY-MM-DD HH:MM:SS", the format used for the created_time and
    // completed_time columns.
    inline std::string current_timestamp_string()
    {
        return std::format("{:%Y-%m-%d %H:%M:%S}", local_now());
    }

    // The inverse of current_timestamp_string(): reads a "YYYY-MM-DD HH:MM:SS"
    // column back as the local wall-clock time it was written as. Returns
    // nullopt when the text does not parse, so the caller decides what a
    // malformed column means rather than silently getting a garbage date.
    inline std::optional<std::chrono::system_clock::time_point>
    parse_local_timestamp(const std::string& text)
    {
        std::chrono::local_seconds parsed;
        std::istringstream in(text);
        in >> std::chrono::parse("%Y-%m-%d %H:%M:%S", parsed);
        if (in.fail())
            return std::nullopt;

        // A stored local time can be ambiguous (the repeated hour when DST
        // falls back) or nonexistent (the skipped hour when it springs
        // forward). The plain to_sys() throws on both; choose::earliest
        // resolves them instead, since a timestamp already on disk should
        // always read back as *some* instant.
        return std::chrono::current_zone()->to_sys(parsed, std::chrono::choose::earliest);
    }
}