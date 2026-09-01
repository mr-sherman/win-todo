/*
TODO:  A windows-native terminal application for managing todo lists.
Copyright 2025 by Bradford Sherman, @mr-sherman

No AI was used in the writing of this application.
I kick it old school.

*/

#include <boost/program_options.hpp>
#include <filesystem>
#include <iostream>
#include "todo_list_db.hpp"
#include "todo_list.hpp"
#include <cstdlib>
#include <sstream>
#include "todo_utils.hpp"



namespace po = boost::program_options;
namespace fs = std::filesystem;

using namespace todo;

namespace {

// USERPROFILE on Windows, HOME everywhere else.
fs::path user_home_directory() {
#ifdef _WIN32
    const char* env = std::getenv("USERPROFILE");
#else
    const char* env = std::getenv("HOME");
#endif
    return env != nullptr ? fs::path(env) : fs::path();
}

}  // namespace

int main (int argc, char * argv[]) {
    std::string config_file, db_file;

    int max_task_length = 0;

    fs::path home = user_home_directory();
    if (home.empty())
    {
        std::cerr << "Cannot find user profile folder." << std::endl;
        return 1;
    }

    fs::path todo_folder = home / ".todo";
    fs::path cfg_path = todo_folder / "todo.cfg";
    fs::path default_db_path = todo_folder / "todo.db";


    po::options_description desc
        ("\nMandatory arguments marked with '*'.\n"
           "Invocation : <program> <command> <arguments \n");

    po::options_description config("Configuration");
        config.add_options()
            ("db_file", po::value<std::string>(&db_file)->default_value(""), 
                  "database file")
            ("max_task_length", 
                 po::value< int >()->default_value(30), 
                 "max task string length")
            ;

    desc.add_options ()
    ("command",  po::value<std::string>()->required(),
                 "* command:  'add', 'delete', `complete`")
    ("arguments",  po::value<std::string>(),
                 "* command arguments")
    ("config,c", po::value<std::string>(&config_file)->default_value("multiple_sources.cfg"),
                  "name of a file of a configuration.");


    // Positional arguments don't need a parameter flag
    po::positional_options_description pos_desc;
    pos_desc.add("command", 1);
    pos_desc.add("arguments", -1);

    boost::program_options::variables_map vm;

    try {
        po::store(boost::program_options::command_line_parser(argc, argv).
                                                              options(desc).
                                                              positional(pos_desc).
                                                              run(), vm);
        po::notify(vm);

    } catch (po::error& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        std::cerr << desc << "\n";
        return 1;
    }
    try {
        if (!fs::exists(todo_folder))
        {
            fs::create_directory(todo_folder);
        }
    }
    catch(fs::filesystem_error  &e)
    {
        std::cerr << e.what() << std::endl;
        return e.code().value();
    }
    std :: string command(vm["command"].as<std::string>());

    try
    {
        db_file = default_db_path.string();
        if (to_upper(command) == "ADD")
        {
            std :: string arguments(vm["arguments"].as<std::string>());
            todo::todo_list_db db (db_file);
            db.create_db();
            db.create_list_entry(arguments);
            std::cout << " ";
        }
        if (to_upper(command) == "LIST")
        {
            todo_list_db db (db_file);
            auto open_items = db.get_open_items();
            std::cout << open_items;
        }
        if (to_upper(command) == "COMPLETE")
        {
            std :: string arguments(vm["arguments"].as<std::string>());
            todo_list_db db (db_file);
            int item_number;
            std::istringstream ( arguments ) >> item_number;
            auto result = db.resolve_list_entry(item_number);
            if (result == 0)
            {
                std::cout << "Task #" << item_number << " marked as completed." << std::endl;
            }
        }
        if (to_upper(command)== "DELETE")
        {
            std :: string arguments(vm["arguments"].as<std::string>());
            todo_list_db db (db_file);
            int item_number;
            std::istringstream ( arguments ) >> item_number;

            auto result = db.delete_list_entry(item_number);
            if (result == 0)
            {
                std::cout << "Task #" << item_number << " removed from task list." << std::endl;
            }
        }

    }
    catch(const todo::todo_error& e)
    {
        std::cerr << e << std::endl;
        return e.error();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    
    return 0;    
}

