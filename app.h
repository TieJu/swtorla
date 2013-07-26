#pragma once

#include <memory>

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

#include "ui_base.h"

#include "dir_watcher.h"
#include "log_processor.h"
#include "combat_analizer.h"

struct program_version {
    int     major;
    int     minor;
    int     patch;
    int     build;
};

class app : boost::noncopyable {
    enum class state {
        update_check,
        update_load,
        update_apply,
        cleanup_update_screen,
        enter_main_screen,
        main_screen,
        shutdown
    };

    state                           _state;
    const char*                     _config_path;
    boost::property_tree::wptree    _config;
    boost::asio::io_service         _io_service;
    std::unique_ptr<ui_base>        _ui;
    program_version                 _version;
    std::unique_ptr<dir_watcher>    _dir_watcher;
    log_processor                   _log_reader;
    string_to_id_string_map         _string_map;
    character_list                  _char_list;
    combat_analizer                 _analizer;

    void log_entry_handler(const combat_log_entry& e_);

    void transit_state(state new_state_);


    std::string check_update();
    std::string download_update(std::string update_path);
    void start_update_process();
    void remove_old_file();

    void read_config(const char* config_path_);
    void write_config(const char* config_path_);

    void send_crashreport(const char* path_);

public:
    app(const char* caption_, const char* config_path_);
    ~app();

    void operator()();
};

