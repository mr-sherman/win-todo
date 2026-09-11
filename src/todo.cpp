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
                  "database file to use instead of ~/.todo/todo.db")
            ;

    desc.add_options ()
    ("command",  po::value<std::string>()->required(),
                 "* command:  'add', 'delete', 'complete', 'edit', 'list', 'export', 'import'")
    ("arguments",  po::value<std::vector<std::string>>()->multitoken(),
                 "* command arguments")
    ("tag,t", po::value<std::string>()->default_value(""),
                 "tag/category: sets it on 'add', updates it on 'edit', filters by it on 'list'/'export'")
    ("all,a", po::bool_switch()->default_value(false),
                 "with 'list': include completed tasks too")
    ("config,c", po::value<std::string>(&config_file)->default_value(cfg_path.string()),
                  "configuration file to read settings from");


    // Without this the Configuration options are declared but never
    // registered, so --db_file is rejected as an unrecognised option.
    desc.add(config);

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
        // Settings not given on the command line fall back to the config
        // file. po::store keeps the first value it sees for an option, so
        // storing the command line first is what makes it win here.
        // Read the name out of vm rather than config_file: po::store does
        // not assign to bound variables, so config_file is still empty
        // until po::notify below.
        std::ifstream cfg_in(vm["config"].as<std::string>());
        if (cfg_in)
            po::store(po::parse_config_file(cfg_in, config), vm);

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

    // "arguments" collects every positional token after the command, since
    // 'edit' needs both a task number and (possibly multi-word) new text.
    // Kept as a vector rather than one std::string so unquoted multi-word
    // text can be typed without wrapping it in quotes.
    std::vector<std::string> arg_tokens =
        vm.count("arguments") ? vm["arguments"].as<std::vector<std::string>>() : std::vector<std::string>();

    auto joined_arguments = [&arg_tokens](std::size_t start = 0) -> std::string
    {
        std::ostringstream oss;
        for (std::size_t i = start; i < arg_tokens.size(); ++i)
        {
            if (i > start)
                oss << ' ';
            oss << arg_tokens[i];
        }
        return oss.str();
    };

    try
    {
        if (db_file.empty())
            db_file = default_db_path.string();
        if (to_upper(command) == "ADD")
        {
            std :: string arguments(joined_arguments());
            std :: string tag(vm["tag"].as<std::string>());
            todo::todo_list_db db (db_file);
            db.create_db();
            db.create_list_entry(arguments, tag);
            std::cout << " ";
        }
        if (to_upper(command) == "LIST")
        {
            std :: string tag(vm["tag"].as<std::string>());
            bool show_all = vm["all"].as<bool>();
            todo_list_db db (db_file);
            db.create_db();
            auto items = show_all ? db.get_all_items(tag) : db.get_open_items(tag);
            std::cout << items;
        }
        if (to_upper(command) == "COMPLETE")
        {
            if (arg_tokens.empty())
                throw todo_error(2, "COMPLETE requires a task number");
            todo_list_db db (db_file);
            int item_number;
            std::istringstream ( arg_tokens[0] ) >> item_number;
            auto result = db.resolve_list_entry(item_number);
            if (result == 0)
            {
                std::cout << "Task #" << item_number << " marked as completed." << std::endl;
            }
        }
        if (to_upper(command)== "DELETE")
        {
            if (arg_tokens.empty())
                throw todo_error(2, "DELETE requires a task number");
            todo_list_db db (db_file);
            int item_number;
            std::istringstream ( arg_tokens[0] ) >> item_number;

            auto result = db.delete_list_entry(item_number);
            if (result == 0)
            {
                std::cout << "Task #" << item_number << " removed from task list." << std::endl;
            }
        }
        if (to_upper(command) == "EDIT")
        {
            if (arg_tokens.empty())
                throw todo_error(2, "EDIT requires a task number");

            int item_number;
            std::istringstream ( arg_tokens[0] ) >> item_number;

            std::optional<std::string> new_text;
            if (arg_tokens.size() > 1)
                new_text = joined_arguments(1);

            std::optional<std::string> new_tag;
            if (!vm["tag"].defaulted())
                new_tag = vm["tag"].as<std::string>();

            if (!new_text && !new_tag)
                throw todo_error(2, "EDIT requires new task text and/or --tag");

            todo_list_db db (db_file);
            auto result = db.edit_list_entry(item_number, new_text, new_tag);
            if (result == 0)
            {
                std::cout << "Task #" << item_number << " updated." << std::endl;
            }
        }
        if (to_upper(command) == "EXPORT")
        {
            if (arg_tokens.empty())
                throw todo_error(2, "EXPORT requires an output file path");

            std :: string arguments(joined_arguments());
            std :: string tag(vm["tag"].as<std::string>());
            todo_list_db db (db_file);
            db.create_db();
            auto open_items = db.get_open_items(tag);

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
            if (arg_tokens.empty())
                throw todo_error(2, "IMPORT requires an input file path");

            std :: string arguments(joined_arguments());
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

