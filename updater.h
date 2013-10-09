#pragma once

#include <string>
#include <http_client.h>

#include <boost/property_tree/ptree.hpp>

class update_dialog;

class updater {
    boost::property_tree::wptree*   _config;

public:
    updater() = default;
    updater( boost::property_tree::wptree& config_ );

    pplx::task<size_t> query_build( update_dialog& ui_ );
    pplx::task<void> download_update( update_dialog& ui_, size_t from_, size_t to_, const std::wstring& target_ );
    pplx::task<std::string> get_patchnotes( size_t from_, size_t to_ );
};