#pragma once

#include <string>
#include <http_client.h>

#include <boost/property_tree/ptree.hpp>

class update_dialog;

struct update_server_info {
    std::wstring    name;
    std::wstring    path;
    size_t          api;
    size_t          latest_version;
};

class updater {
    boost::property_tree::wptree*   _config;

    pplx::task<update_server_info> get_update_server_info( const std::wstring& name_, const std::wstring& path_, size_t api_ );

public:
    updater() = default;
    updater( boost::property_tree::wptree& config_ );
    void config( boost::property_tree::wptree& config_ );

    pplx::task<update_server_info> get_best_update_server();
    pplx::task<size_t> download_update( const update_server_info& info_, size_t from_, size_t to_, const std::wstring& target_ );
    pplx::task<std::wstring> download_patchnotes( const update_server_info& info_, size_t from_, size_t to_ );
};