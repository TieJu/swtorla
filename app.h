#pragma once

#include <memory>
#include <future>

#include <boost/property_tree/ptree.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <boost/asio.hpp>

#include <boost/noncopyable.hpp>

#include <Windows.h>

#include "window.h"

#include "main_ui.h"

#include "dir_watcher.h"
#include "log_processor.h"
#include "combat_analizer.h"
#include "update_dialog.h"

class app : boost::noncopyable {
    const char*                     _config_path;
    boost::property_tree::wptree    _config;
    boost::asio::io_service         _io_service;
    std::unique_ptr<main_ui>        _ui;
    program_version                 _version;
    std::unique_ptr<dir_watcher>    _dir_watcher;
    log_processor                   _log_reader;
    string_to_id_string_map         _string_map;
    character_list                  _char_list;
    combat_analizer                 _analizer;

    std::string load_update_info(const std::string& name_);
    std::future<void> show_update_info(const std::string& name_);
    bool run_update_async_job(update_dialog& dlg_);
    bool run_update_async();
    std::future<bool> run_update();

    void setup_from_config();
    void log_entry_handler(const combat_log_entry& e_);

    std::string check_update(update_dialog& dlg_);
    std::string download_update(update_dialog& dlg_, std::string update_path);
    void start_update_process(update_dialog& dlg_);
    void remove_old_file();

    void read_config(const char* config_path_);
    void write_config(const char* config_path_);

    void send_crashreport(const char* path_);

public:
    app(const char* caption_, const char* config_path_);
    ~app();

    void operator()();

protected:
    friend class main_ui;
    void set_log_dir(const wchar_t* path_);
    std::wstring get_log_dir();
};

