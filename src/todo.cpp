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
#include <fstream>
#include <regex>
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

// Matches a markdown checklist line, e.g. "- [x] buy milk <!-- id:3 -->".
// Group 1 is the mark inside the brackets, group 2 is the item text
// (unused), group 3 is the row id from the id comment, if present.
const std::regex checklist_line_re(
    R"(^\s*[-*+]\s+\[([ xX])\]\s*(.*?)\s*(?:<!--\s*id:(\d+)\s*-->\s*)?$)");

// Reads a markdown checklist file and closes every checked item that
// carries an `<!-- id:N -->` comment, where N is the SQLite rowid captured
// at export time (see todo_list::to_markdown_checklist_line). rowid is
// stable across renumbering and repeated imports, unlike the visible task
// number, so resolve_by_rowid always closes the item that was actually
// checked, however many other completions have happened since export.
// Checked items with no id comment (hand-written checklists) are reported
// and skipped, since there's no reliable way to map them back to a row.
int import_markdown_checklist(todo::todo_list_db& db, const std::string& file_path)
{
    std::ifstream in(file_path);
    if (!in)
    {
        throw todo::todo_error(4, "Could not open import file: " + file_path);
    }

    int closed_count = 0;
    std::string line;
    while (std::getline(in, line))
    {
        std::smatch m;
        if (!std::regex_match(line, m, checklist_line_re))
            continue;

        bool checked = (m[1].str() == "x" || m[1].str() == "X");
        if (!checked)
            continue;

        if (!m[3].matched)
        {
            std::cerr << "Skipping checked item with no id comment: " << m[2].str() << std::endl;
            continue;
        }

        long long row_id = std::stoll(m[3].str());
        db.resolve_by_rowid(row_id);
        ++closed_count;
    }

    return closed_count;
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
                 "* command:  'add', 'delete', 'complete', 'list', 'export', 'import'")
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
        if (to_upper(command) == "EXPORT")
        {
            std :: string arguments(vm["arguments"].as<std::string>());
            todo_list_db db (db_file);
            auto open_items = db.get_open_items();

            std::ofstream out(arguments);
            if (!out)
            {
                throw todo_error(4, "Could not open export file: " + arguments);
            }
            out << to_markdown_export(open_items, current_date_string());
            std::cout << "Exported " << open_items.size() << " item(s) to " << arguments << std::endl;
        }
        if (to_upper(command) == "IMPORT")
        {
            std :: string arguments(vm["arguments"].as<std::string>());
            todo_list_db db (db_file);
            int closed_count = import_markdown_checklist(db, arguments);
            std::cout << "Processed " << closed_count << " checked item(s) from " << arguments << std::endl;
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

